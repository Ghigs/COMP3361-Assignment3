/*
 * Process - execute memory trace file
 * 
 * COMP3361 Winter 2019 Program 2 Sample Solution
 *
 * File:   Process.h
 * Author: Mike Goss <mikegoss@cs.du.edu>
 */

#ifndef PROCESS_H
#define PROCESS_H

#include "PageTableManager.h"

#include <MMU.h>

#include <fstream>
#include <memory>
#include <string>
#include <vector>

class Process;

class PageFaultHandler : public mem::MMU::FaultHandler {
public:
    PageFaultHandler(Process* parent_process):process{parent_process} {};
    bool Run(const mem::PMCB &pmcb);
private:
    Process* process;
};

class WritePermissionFaultHandler : public mem::MMU::FaultHandler {
public:
    WritePermissionFaultHandler(Process* parent_process):process{parent_process} {};
    bool Run(const mem::PMCB &pmcb);
private:
    Process* process;
};

class Process {
public:
  /**
   * Constructor - open trace file, initialize processing.
   *   Must be called in kernel mode.
   * 
   * @param file_name_ source of trace commands
   * @param memory MMU class object to use for memory
   * @param ptm page table manager
   */
  Process(const std::string &file_name_, mem::MMU &memory_, PageTableManager &ptm_);
  
  /**
   * Destructor - close trace file, clean up processing
   */
  ~Process();

  // Other constructors, assignment
  Process(const Process &other) = delete;
  Process(Process &&other) = delete;
  Process operator=(const Process &other) = delete;
  Process operator=(Process &&other) = delete;
  
  /**
   * Exec - read and process commands from trace file
   * 
   */
  bool Exec(int num_Commands);
  
  
  void Alloc(const mem::PMCB &pmcb, 
                mem::Addr vaddr, 
                size_t count);
  
  uint32_t getQuota();
  uint32_t getPageFrameCount() { return page_frame_count; }
  
private:
  // Trace file
  std::string file_name;
  std::fstream trace;
  long line_number;
  uint32_t quota;
  uint32_t page_frame_count;

  // Memory contents
  mem::MMU &memory;
  mem::PMCB proc_pmcb;  // PMCB for this process
  
  // Page table access
  PageTableManager &ptm;
  
  const std::shared_ptr<PageFaultHandler> pfh = std::make_shared<PageFaultHandler>(PageFaultHandler(this)); // PageFaultHandler for this process
  const std::shared_ptr<WritePermissionFaultHandler> wpfh = std::make_shared<WritePermissionFaultHandler>(WritePermissionFaultHandler(this)); // WritePermissionFaultHandler for this process
  
  /**
   * ParseCommand - parse a trace file command.
   *   Aborts program if invalid trace file.
   * 
   * @param line return the original command line
   * @param cmd return the command name
   * @param cmdArgs returns a vector of argument values
   * @return true if command parsed, false if end of file
   */
  bool ParseCommand(
      std::string &line, std::string &cmd, std::vector<uint32_t> &cmdArgs);
  
  /**
   * Command executors. Arguments are the same for each command.
   *   Form of the function is CmdX, where "X' is the command name, capitalized.
   * @param line original text of command line
   * @param cmd command, converted to all lower case
   * @param cmdArgs arguments to command
   */
  
  void CmdQuota(const std::string &line, 
                const std::string &cmd, 
                const std::vector<uint32_t> &cmdArgs);
  void CmdCmp(const std::string &line, 
              const std::string &cmd, 
              const std::vector<uint32_t> &cmdArgs);
  void CmdSet(const std::string &line, 
              const std::string &cmd, 
              const std::vector<uint32_t> &cmdArgs);
  void CmdFill(const std::string &line, 
               const std::string &cmd, 
               const std::vector<uint32_t> &cmdArgs);
  void CmdDup(const std::string &line, 
              const std::string &cmd, 
              const std::vector<uint32_t> &cmdArgs);
  void CmdPrint(const std::string &line, 
                const std::string &cmd, 
                const std::vector<uint32_t> &cmdArgs);
  void CmdPerm(const std::string &line, 
               const std::string &cmd, 
               const std::vector<uint32_t> &cmdArgs);
};


#endif /* PROCESS_H */

