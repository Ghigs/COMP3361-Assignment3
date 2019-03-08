/* PageTableManager - page table functionality for COMP3361 Winter 2019
 *   Program Assignment 2 Sample Solution
 *  
 * File:   PageTableManager.h
 * Author: Mike Goss <mikegoss@cs.du.edu>
 */

#ifndef PAGETABLEMANAGER_H
#define PAGETABLEMANAGER_H

#include "FrameAllocator.h"

class PageTableManager {
public:
  /**
   * Constructor - build kernel page table and enter virtual mode
   * 
   * Must be in physical memory mode on entry. Creates kernel page table in MMU, 
   * enters virtual mode.
   * 
   * @param memory_ MMU class object to use for memory
   * @param allocator_ page frame allocator object
   * @throws std::runtime_error if unable to allocate memory for page table
   */
  PageTableManager(mem::MMU &memory_, FrameAllocator &allocator_);
  virtual ~PageTableManager() {}  // empty destructor
  
  // Disallow copy/move
  PageTableManager(const PageTableManager &other) = delete;
  PageTableManager(PageTableManager &&other) = delete;
  PageTableManager &operator=(const PageTableManager &other) = delete;
  PageTableManager &operator=(PageTableManager &&other) = delete;
  
  /**
   * CreateProcessPageTable - create empty page table for process
   * 
   * All entries in the page table will be 0 (not present). Must be called
   * in kernel mode.
   * 
   * @return kernel address of new page table
   * @throws std::runtime_error if unable to allocate memory for page table
   */
  mem::Addr CreateProcessPageTable(void);
  
  /**
   * MapProcessPages - map pages into the memory of specified process
   * 
   * The requested pages are allocated and mapped into the page table of
   * the process whose PMCB is specified. Any pages
   * already mapped are ignored. Must be called in kernel mode.
   * 
   * @param user_pmcb PMCB of process to modify
   * @param vaddr starting virtual address
   * @param count number of pages to map
   * @throws std::runtime_error if unable to allocate memory for pages
   */
  void MapProcessPages(const mem::PMCB &user_pmcb, mem::Addr vaddr, size_t count);
  
  /**
   * SetPageWritePermission - change writable bit for page(s)
   * 
   * Must be called in kernel mode.
   * 
   * @param user_pmcb PMCB of process to modify
   * @param vaddr starting virtual address
   * @param count number of pages to change
   * @param writable non-zero to set writable bit, 0 to clear it
   */
  void SetPageWritePermission(
      const mem::PMCB &user_pmcb, mem::Addr vaddr, size_t count, uint32_t writable);
  
 private:
   // Save references to memory and allocator
   mem::MMU &memory;
   FrameAllocator &allocator;
};

#endif /* PAGETABLEMANAGER_H */

