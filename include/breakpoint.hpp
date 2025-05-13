#ifndef HARODBG_BREAKPOINT_HPP
#define HARODBG_BREAKPOINT_HPP

#include<cstdint>
#include<sys/ptrace.h>
#include<sys/types.h>



namespace harodbg{
    class breakpoint{
    
    public:
        breakpoint() = default;
        breakpoint(pid_t pid, std::intptr_t addr) : m_pid{pid}, m_addr{addr}, m_enabled{false}, m_saved_data{} {}

        void enable(){
            //peekdata => word(2 bytes)단위로 읽기.
            auto data = ptrace(PTRACE_PEEKDATA, m_pid, m_addr, nullptr);
            //데이터 복사 1바이트 빼고
            m_saved_data = static_cast<uint8_t>(data & ~0xff);
            uint64_t int3 = 0xcc;
            uint64_t data_with_int3 = ((data & ~0xff) | int3);
            //breakpoint inturrupt 삽입
            //pokedata => 데이터 삽입
            ptrace(PTRACE_POKEDATA, m_pid, m_addr, data_with_int3);

            m_enabled = true;
        }

        void disable() {
            auto data = ptrace(PTRACE_PEEKDATA, m_pid, m_addr, nullptr);
            auto resotred_data = ((data&~0xff) | m_saved_data);
            ptrace(PTRACE_POKEDATA, m_pid, m_addr, resotred_data);

            m_enabled = false;
        }


        //찾아보니 const가 객체 상태 변경 안함이라는 뜻.
        bool is_enabled() const {return m_enabled;}
        
        // 반환 타입 설정 코틀린이랑 비슷한 부분이 있는듯?
        auto get_address() const -> std::intptr_t {return m_addr;}


    private:
        pid_t m_pid;
        std::intptr_t m_addr;
        bool m_enabled;
        uint8_t m_saved_data;
    };
}


#endif