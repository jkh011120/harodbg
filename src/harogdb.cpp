#include<vector>
#include<sys/ptrace.h>
#include<sys/wait.h>
#include<sys/personality.h>
#include<unistd.h>
#include<sstream>
#include<fstream>
#include<iostream>
#include<iomanip>

#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>

#include "linenoise.h"

#include "/home/ubuntu/harogdb/include/debugger.hpp"

using namespace harodbg;


//delimiter(구분자) 기준으로 split 
std::vector<std::string> split(const std::string &s, char delimiter){
    std::vector<std::string> out{};
    std::stringstream ss {s};
    std::string item;

    while(std::getline(ss,item,delimiter)){
        out.push_back(item);
    }

    return out;
}

//접두사인지 확인
bool is_prefix(const std::string& s, const std::string& of){
    if(s.size() > of.size()) return false;
    return std::equal(s.begin(), s.end(), of.begin());
}

void debugger::handle_command(const std::string& line){
    auto args = split(line,' ');
    auto command = args[0];

    if(is_prefix(command, "continue")){
        continue_execution();
    }
    else if(is_prefix(command, "break")){
        std::string addr {args[1],2};
        set_breakpoint_at_address(std::stol(addr,0,16));
    }
    else{
        std::cerr << "Unknown command\n";
    }
}

void debugger::set_breakpoint_at_address(std::intptr_t addr){
    std::cout << "Set breakpoint at address 0x" << std::hex << addr << std::endl;
    breakpoint bp {m_pid, addr};
    bp.enable();
    m_breakpoints[addr] = bp;
}

void debugger::run(){
    int wait_status;
    auto options = 0;
    waitpid(m_pid, &wait_status, options);

    char* line = nullptr;
    while((line = linenoise("minidbg> ")) != nullptr){
        handle_command(line);
        linenoiseHistoryAdd(line);
        linenoiseFree(line);
    }
}

void debugger::continue_execution(){
    //ptrace (다음 이벤트 전까지 실행용.) 
    ptrace(PTRACE_CONT, m_pid, nullptr,nullptr);
    
    int wait_status;
    auto options = 0;
    //멈추기
    waitpid(m_pid, &wait_status, options);
}

void execute_debugee (const std::string& prog_name){
    if(ptrace(PTRACE_TRACEME, 0, 0, 0) < 0){
        std::cerr << "Error in ptrace\n";
        return;
    }
    execl(prog_name.c_str(), prog_name.c_str(), nullptr);
}

int main(int argc, char* argv[]){
    if(argc <2){
        std::cerr << "Program name not specifiied";
        return -1;
    }

    auto prog = argv[1];

    auto pid = fork();
    
    //child process
    if(pid == 0){
        personality(ADDR_NO_RANDOMIZE);
        execute_debugee(prog);
    }


    //parent process
    else if (pid >= 1){

    std::cout << "Started debugging procees " << pid << "\n";
    debugger dbg{prog, pid};
    dbg.run();
    }

    
}