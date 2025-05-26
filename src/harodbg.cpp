#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <sys/personality.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

// 절대 경로 쓰면 안되는데 빨간줄 미칠거같아서 일단 이렇게함,, ㅠ
#include "/home/haro001/harodbg/ext/linenoise/linenoise.h"
#include "/home/haro001/harodbg/include/debugger.hpp"

using namespace harodbg;

uint64_t debugger::read_memory(uint64_t address) {
  // address에 데이터 받아오기
  return ptrace(PTRACE_PEEKDATA, m_pid, address, nullptr);
}

void debugger::write_memory(uint64_t address, uint64_t value) {
  // address에 value 쓰기
  ptrace(PTRACE_POKEDATA, m_pid, address, value);
}

uint64_t debugger::get_pc() { return get_register_value(m_pid, reg::rip); }

void debugger::set_pc(uint64_t pc) { set_register_value(m_pid, reg::rip, pc); }

void debugger::step_over_breakpoint() {
  auto possible_break_point_location = get_pc() - 1; // 직전에 멈춰야함

  if (m_breakpoints.count(possible_break_point_location)) {
    auto &bp = m_breakpoints[possible_break_point_location]; // map형식 조회
    if (bp.is_enabled()) {

      auto previous_instruction_address = possible_break_point_location;
      set_pc(previous_instruction_address); // 해당위치에서 레지스터 값 저장
      bp.disable();                         // 실행하기
      ptrace(PTRACE_SINGLESTEP, m_pid, nullptr,
             nullptr); // 한줄실행 (bp 걸린 곳)
      wait_for_signal();
      bp.enable(); // 명령어 한줄하고 다시 브레이크
    }
  }
}

void debugger::wait_for_signal() {
  int wait_status;
  auto options = 0;
  waitpid(m_pid, &wait_status, options);
}

void debugger::continue_execution() {
  step_over_breakpoint();
  ptrace(PTRACE_CONT, m_pid, nullptr, nullptr);
  wait_for_signal();
}

void debugger::dump_registers() {
  // 레지스터 이름 : 값 쌍으로
  for (const auto &rd : g_register_descriptors) {
    std::cout << rd.name << " 0x" << std::setfill('0') << std::setw(16)
              << std::hex << get_register_value(m_pid, rd.r) << std::endl;
  }
}

// delimiter(구분자) 기준으로 split
std::vector<std::string> split(const std::string &s, char delimiter) {
  std::vector<std::string> out{};
  std::stringstream ss{s};
  std::string item;

  while (std::getline(ss, item, delimiter)) {
    out.push_back(item);
  }

  return out;
}

// 접두사인지 확인
bool is_prefix(const std::string &s, const std::string &of) {
  if (s.size() > of.size())
    return false;
  return std::equal(s.begin(), s.end(), of.begin());
}

void debugger::handle_command(const std::string &line) {
  auto args = split(line, ' ');
  auto command = args[0];

  if (is_prefix(command, "continue")) {
    continue_execution();
  } else if (is_prefix(command, "break")) {
    std::string addr{args[1], 2};
    set_breakpoint_at_address(std::stol(addr, 0, 16));
  } else if (is_prefix(command, "register")) {
    if (is_prefix(args[1], "dump")) {
      dump_registers();
    } else if (is_prefix(args[1], "read")) {
      std::cout << get_register_value(m_pid, get_register_from_name(args[2]))
                << std::endl;
    } else if (is_prefix(args[1], "write")) {
      std::string val{args[3], 2}; // assume 0xVAL
      set_register_value(m_pid, get_register_from_name(args[2]),
                         std::stol(val, 0, 16));
    }
  } else if (is_prefix(command, "memory")) {
    std::string addr{args[2], 2}; // assume 0xADDRESS

    if (is_prefix(args[1], "read")) {
      std::cout << std::hex << read_memory(std::stol(addr, 0, 16)) << std::endl;
    }
    if (is_prefix(args[1], "write")) {
      std::string val{args[3], 2}; // assume 0xVAL
      write_memory(std::stol(addr, 0, 16), std::stol(val, 0, 16));
    }
  } else {
    std::cerr << "Unknown command\n";
  }
}

void debugger::set_breakpoint_at_address(std::intptr_t addr) {
  std::cout << "Set breakpoint at address 0x" << std::hex << addr << std::endl;
  breakpoint bp{m_pid, addr};
  bp.enable();
  m_breakpoints[addr] = bp;
}

void debugger::run() {
  int wait_status;
  auto options = 0;
  waitpid(m_pid, &wait_status, options);

  char *line = nullptr;
  while ((line = linenoise("minidbg> ")) != nullptr) {
    handle_command(line);
    linenoiseHistoryAdd(line);
    linenoiseFree(line);
  }
}

void execute_debugee(const std::string &prog_name) {
  if (ptrace(PTRACE_TRACEME, 0, 0, 0) < 0) {
    std::cerr << "Error in ptrace\n";
    return;
  }
  execl(prog_name.c_str(), prog_name.c_str(), nullptr);
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Program name not specifiied";
    return -1;
  }

  auto prog = argv[1];

  auto pid = fork();

  // child process
  if (pid == 0) {
    personality(ADDR_NO_RANDOMIZE);
    execute_debugee(prog);
  }

  // parent process
  else if (pid >= 1) {

    std::cout << "Started debugging procees " << pid << "\n";
    debugger dbg{prog, pid};
    dbg.run();
  }
}
