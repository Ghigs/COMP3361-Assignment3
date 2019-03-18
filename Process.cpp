/*
 * Process implementation 
 * 
 * COMP3361 Winter 2019 Program 2 Sample Solution
 * 
 * File:   Process.cpp
 * Author: Mike Goss <mikegoss@cs.du.edu>
 * 
 * 
 */

#include "Process.h"
#include "PageTableManager.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>

using mem::Addr;
using mem::kPageSize;
using std::cin;
using std::cout;
using std::cerr;
using std::getline;
using std::istringstream;
using std::string;
using std::vector;

namespace {

/**
 * PageFaultHandler
 */
class PageFaultHandler : public mem::MMU::FaultHandler {
public:
  /**
   * Run - handle fault
   * 
   * Print message and return false
   * 
   * @param pmcb user mode pmcb
   * @return false
   */
  virtual bool Run(const mem::PMCB &pmcb) {
    cout << ((pmcb.operation_state == mem::PMCB::WRITE_OP) ? "Write " : "Read ")
           << "Page Fault at address " << std::setw(7) << std:: setfill('0') 
           << std::hex << pmcb.next_vaddress << "\n";
    
    
    // This should allocate new pages based on demand.
   // Alloc(pmcb, pmcb.next_vaddress, 1); 
    
    // This was originally false, but i think returning false forces us to quit, which we don't want if we are trying to allocate on demand
    return false;
  }
};

/**
 * WritePermissionFaultHandler
 */
class WritePermissionFaultHandler : public mem::MMU::FaultHandler {
public:
  /**
   * Run - handle fault
   * 
   * Print message and return false
   * 
   * @param pmcb user mode pmcb
   * @return false
   */
  virtual bool Run(const mem::PMCB &pmcb) {
    cout << "Write Permission Fault at address " << std::setw(7) << std:: setfill('0') 
            << std::hex << pmcb.next_vaddress << "\n";
    return false;
  }
};



}

Process::Process(const string &file_name_, mem::MMU &memory_, PageTableManager &ptm_) 
: file_name(file_name_), line_number(0), memory(memory_), ptm(ptm_) {
  // Open the trace file.  Abort program if can't open.
  trace.open(file_name, std::ios_base::in);
  if (!trace.is_open()) {
    cerr << "ERROR: failed to open trace file: " << file_name << "\n";
    exit(2);
  }
  
  // Set up empty process page table and load process PMCB
  proc_pmcb.page_table_base = ptm.CreateProcessPageTable();
  
  // Attempting to create Process specific fault handlers
  //pfh = std::make_shared<PageFaultHandler>();
  //wpfh = std::make_shared<WritePermissionFaultHandler>();
  
}

Process::~Process() {
  trace.close();
}

bool Process::Exec(int num_Commands) {
  // Set the user page table
  memory.set_user_PMCB(proc_pmcb);
  
  // Set up fault handlers
 // memory.SetPageFaultHandler(pfh);
 // memory.SetPageFaultHandler(wpfh);
  memory.SetPageFaultHandler(std::make_shared<PageFaultHandler>());
  memory.SetWritePermissionFaultHandler(std::make_shared<WritePermissionFaultHandler>());
  
  // Read and process commands
  string line;                // text line read
  string cmd;                 // command from line
  vector<uint32_t> cmdArgs;   // arguments from line
  
  // Select the command to execute
  while (ParseCommand(line, cmd, cmdArgs)) {
    if (cmd == "quota" ) {
      CmdQuota(line, cmd, cmdArgs);      // allocate and map pages
    } else if (cmd == "cmp") {
      CmdCmp(line, cmd, cmdArgs);        // get and compare multiple bytes
    } else if (cmd == "set") {
      CmdSet(line, cmd, cmdArgs);        // put bytes
    } else if (cmd == "fill") {
      CmdFill(line, cmd, cmdArgs);       // fill bytes with value
    } else if (cmd == "dup") {
      CmdDup(line, cmd, cmdArgs);        // duplicate bytes to dest from source
    } else if (cmd == "print") {
      CmdPrint(line, cmd, cmdArgs);      // dump byte values to output
    } else if (cmd == "perm") {
      CmdPerm(line, cmd, cmdArgs);       // change pages' writable bits
    } else if (cmd != "*") {
      cerr << "ERROR: invalid command\n";
      exit(2);
    }
  }
}

bool Process::ParseCommand(
    string &line, string &cmd, vector<uint32_t> &cmdArgs) {
  cmdArgs.clear();
  line.clear();
  
  // Read next line
  if (std::getline(trace, line)) {
    ++line_number;
    cout << std::dec << line_number << ":" << line << "\n";
    
    // No further processing if comment or empty line
    if (line.size() == 0 || line[0] == '*') {
      cmd = "*";
      return true;
    }
    
    // Make a string stream from command line
    istringstream lineStream(line);
    
    // Get address
    uint32_t addr = 0;
    if (!(lineStream >> std::hex >> addr)) {
      if (lineStream.eof()) {
        // Blank line, treat as comment
        cmd = "*";
        return true;
      } else {
        cerr << "ERROR: badly formed address in trace file: "
                << file_name << " at line " << line_number << "\n";
        exit(2);
      }
    }
    cmdArgs.push_back(addr);
    
    // Get command
    if (!(lineStream >> cmd)) {
      cerr << "ERROR: no command name following address in trace file: "
              << file_name << " at line " << line_number << "\n";
      exit(2);
    }
    
    // Get any additional arguments
    Addr arg;
    while (lineStream >> std::hex >> arg) {
      cmdArgs.push_back(arg);
    }
    return true;
  } else if (trace.eof()) {
      return false;
  } else {
    cerr << "ERROR: getline failed on trace file: " << file_name 
            << " at line " << line_number << "\n";
    exit(2);
  }
}


void Process::Alloc(mem::PMCB &pmcb,
                    Addr vaddr, 
                    size_t count) {
  // Allocate the specified memory pages  
  memory.set_kernel_PMCB();
  ptm.MapProcessPages(pmcb, vaddr, count);
  memory.set_user_PMCB(pmcb);
}


void Process::CmdQuota(const string &line,
                       const string &cmd,
                       const vector<uint32_t> &cmdArgs) {
    uint32_t count = cmdArgs.at(0);
    quota = count;
}


void Process::CmdCmp(const string &line,
                     const string &cmd,
                     const vector<uint32_t> &cmdArgs) {
  if (cmdArgs.size() != 3) {
    cerr << "ERROR: badly formatted cmp command\n";
    exit(2);
  }
  Addr addr1 = cmdArgs.at(0);
  Addr addr2 = cmdArgs.at(1);
  uint32_t count = cmdArgs.at(2);

  // Compare specified byte values
  for (uint32_t i = 0; i < count; ++i) {
    Addr a1 = addr1 + i;
    uint8_t v1 = 0;
    if (!memory.movb(&v1, a1)) return;
    Addr a2 = addr2 + i;
    uint8_t v2 = 0;
    if (!memory.movb(&v2, a2)) return;
    if(v1 != v2) {
      cout << std::setfill('0') << std::hex
              << "cmp error"
              << ", addr1 = "  << std::setw(7) << a1
              << ", value = " << std::setw(2) << static_cast<uint32_t>(v1)
              << ", addr2 = "  << std::setw(7) << a2
              << ", value = " << std::setw(2) << static_cast<uint32_t>(v2) << "\n";
    }
  }
}

void Process::CmdSet(const string &line,
                     const string &cmd,
                     const vector<uint32_t> &cmdArgs) {
  // Store multiple bytes starting at specified address
  Addr addr = cmdArgs.at(0);
  for (int i = 1; i < cmdArgs.size(); ++i) {
    uint8_t b = cmdArgs.at(i);
    if (!memory.movb(addr++, &b)) return;
  }
}

void Process::CmdDup(const string &line,
                     const string &cmd,
                     const vector<uint32_t> &cmdArgs) {
  if (cmdArgs.size() != 3) {
    cerr << "ERROR: badly formatted dup command\n";
    exit(2);
  }
  
  // Copy specified number of bytes to destination from source
  Addr dst = cmdArgs.at(1);
  Addr src = cmdArgs.at(0);
  uint32_t count = cmdArgs.at(2);
  
  // Copy a byte at a time in case of page fault
  uint8_t buffer;
  while (count > 0) {
    if (!memory.movb(&buffer, src++)) return;
    if (!memory.movb(dst++, &buffer)) return;
    --count;
  }
}

void Process::CmdFill(const string &line,
                     const string &cmd,
                     const vector<uint32_t> &cmdArgs) {
  // Fill destination range with specified value
  uint8_t value = cmdArgs.at(1);
  uint32_t count = cmdArgs.at(2);
  Addr addr = cmdArgs.at(0);
  
  // Use buffer for efficiency
  uint8_t buffer[1024];
  memset(buffer, value, std::min(count, sizeof(buffer)));
  
  // Write data to memory
  while (count > 0) {
    uint32_t block_size = std::min(count, sizeof(buffer));
    if (!memory.movb(addr, buffer, block_size)) return;
    addr += block_size;
    count -= block_size;
  }
}

void Process::CmdPrint(const string &line,
                     const string &cmd,
                     const vector<uint32_t> &cmdArgs) {
  Addr addr = cmdArgs.at(0);
  uint32_t count = cmdArgs.at(1);

  // Output the specified number of bytes starting at the address
  for (int i = 0; i < count; ++i) {
    if ((i % 16) == 0) { // Write new line with address every 16 bytes
      if (i > 0) cout << "\n";  // not before first line
      cout << std::hex << std::setw(7) << std::setfill('0') << addr << ":";
    }
    uint8_t b;
    if (!memory.movb(&b, addr++)) return;
    cout << " " << std::setfill('0') << std::setw(2) << static_cast<uint32_t> (b);
  }
  cout << "\n";
}

void Process::CmdPerm(const string &line, 
                      const string &cmd, 
                      const vector<uint32_t> &cmdArgs) {
  // Change the permissions of the specified pages
  memory.set_kernel_PMCB();
  ptm.SetPageWritePermission(proc_pmcb, cmdArgs.at(0), cmdArgs.at(1), cmdArgs.at(2));
  memory.set_user_PMCB(proc_pmcb);
}

uint32_t Process::getQuota() {
    return quota;
}
