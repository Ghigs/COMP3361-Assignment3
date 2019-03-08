/* PageTableManager - page table functionality for COMP3361 Winter 2019
 *   Program Assignment 2 Sample Solution
 *  
 * File:   PageTableManager.h
 * Author: Mike Goss <mikegoss@cs.du.edu>
 */

#include "PageTableManager.h"

#include <stdexcept>
#include <vector>

using mem::Addr;
using mem::PageTable;
using mem::PageTableEntry;
using mem::PMCB;
using std::vector;

PageTableManager::PageTableManager(mem::MMU &memory_, FrameAllocator &allocator_) 
: memory(memory_), allocator(allocator_) {
  // Allocate page for kernel page table
  vector<Addr> allocated;
  if (!allocator.AllocatePageFrames(1, allocated) || allocated.size() != 1) {
    throw std::runtime_error("Allocation of kernel page table failed");
  }
  
  Addr kernel_page_table = allocated[0];
  
  // Initialize the kernel page table
  PageTable page_table; // local copy of page table to build, initialized to 0
  Addr num_pages = memory.get_frame_count(); // size of physical memory

  // Build page table entries (map to end of existing memory)
  for (Addr i = 0; i < num_pages; ++i) {
    page_table[i] = (i << mem::kPageSizeBits) 
            | mem::kPTE_PresentMask | mem::kPTE_WritableMask;
  }

  // Write page table to MMU memory
  memory.movb(kernel_page_table, &page_table, mem::kPageTableSizeBytes);
  
  // Set page table and enter virtual mode
  mem::PMCB kernel_pmcb(kernel_page_table);
  memory.enter_virtual_mode(kernel_pmcb);
}

Addr PageTableManager::CreateProcessPageTable(void) {
  vector<Addr> allocated;
  if (!allocator.AllocatePageFrames(1, allocated) || allocated.size() != 1) {
    throw std::runtime_error("Allocation of process page table failed");
  }
  
  // FrameAllocator guarantees that pages are all 0, which is what we need
  // for an empty page table, so no further processing is necessary.
  return allocated[0];
}

void PageTableManager::MapProcessPages(
    const mem::PMCB &user_pmcb, Addr vaddr, size_t count) {
  // Check for valid virtual address
  if ((vaddr & mem::kPageOffsetMask) != 0) {
    throw std::runtime_error("vaddr not a multiple of page size");
  }
  
  // Allocate pages
  vector<Addr> allocated;
  if (!allocator.AllocatePageFrames(count, allocated) 
          || allocated.size() != count) {
    throw std::runtime_error("Allocation of process pages failed");
  }
  
  // Map the allocated pages
  vector<Addr> already_mapped;  // any pages that are already mapped
  Addr next_vaddr = vaddr;
  while (!allocated.empty()) {
    Addr pt_index = next_vaddr >> mem::kPageSizeBits;
    if (pt_index >= mem::kPageTableEntries) {
      throw std::runtime_error("Virtual address too large");
    }
    
    // Read existing page table entry
    PageTableEntry pt_entry;
    Addr pte_addr = user_pmcb.page_table_base + pt_index*sizeof(PageTableEntry);
    memory.movb(&pt_entry, pte_addr, sizeof(PageTableEntry));
    
    // If page not mapped, create and write page table entry
    if ((pt_entry & mem::kPTE_PresentMask) == 0) {
      pt_entry = 
              allocated.back() | mem::kPTE_PresentMask | mem::kPTE_WritableMask;
      allocated.pop_back();
      memory.movb(pte_addr, &pt_entry, sizeof(PageTableEntry));
    } else {
      // Duplicate - free the page frame after we're done
      already_mapped.push_back(allocated.back());
      allocated.pop_back();
    }
    
    next_vaddr += mem::kPageSize;
  }
  
  // Release any duplicate pages
  if (!already_mapped.empty()) {
    allocator.FreePageFrames(already_mapped.size(), already_mapped);
  }
}

void PageTableManager::SetPageWritePermission(const mem::PMCB &user_pmcb,
                                              mem::Addr vaddr, 
                                              size_t count, 
                                              uint32_t writable) {
  // Check for valid virtual address
  if ((vaddr & mem::kPageOffsetMask) != 0) {
    throw std::runtime_error("vaddr not a multiple of page size");
  }
  
  // Modify each page which is present
  Addr pt_index = vaddr >> mem::kPageSizeBits;
  for (Addr i = 0; i < count; ++i) {    
    if ((pt_index+i) >= mem::kPageTableEntries) {
      throw std::runtime_error("Virtual address too large");
    }

    // Read existing page table entry
    PageTableEntry pt_entry;
    Addr pte_addr = user_pmcb.page_table_base + (pt_index+i)*sizeof(PageTableEntry);
    memory.movb(&pt_entry, pte_addr, sizeof(PageTableEntry));
    
    // If entry is present, modify the writable bit
    if ((pt_entry & mem::kPTE_PresentMask) != 0) {
      PageTableEntry new_pt_entry = (pt_entry & ~mem::kPTE_WritableMask)
              | (writable ? mem::kPTE_WritableMask : 0);
      if (pt_entry != new_pt_entry) { // if changed
        memory.movb(pte_addr, &new_pt_entry, sizeof(PageTableEntry));
      }
    }
  }
}
