#ifndef HARODBG_DEBUGGER_HPP
#define HARODBG_DEBUGGER_HPP

#include <cstdint>
#include <linux/types.h>
#include <string>
#include <unordered_map>
#include <utility>

#include "breakpoint.hpp"
#include "registers.hpp"

namespace harodbg {
class debugger {
public:
  debugger(std::string prog_name, pid_t pid)
      : m_prog_name{std::move(prog_name)}, m_pid{pid} {}

  void run();
  void set_breakpoint_at_address(std::intptr_t addr);
  void dump_registers();

private:
  void handle_command(const std::string &line);
  void continue_execution();
  auto get_pc(uint64_t pc);
  void step_over_breakpoint();
  void wait_for_signal();
  auto get_pc() -> uint64_t;
  void set_pc(uint64_t pc);
  auto read_memory(uint64_t address) -> uint64_t;
  void write_memory(uint64_t address, uint64_t value);

  std::string m_prog_name;
  pid_t m_pid;
  std::unordered_map<std::intptr_t, breakpoint> m_breakpoints;
};

} // namespace harodbg

#endif