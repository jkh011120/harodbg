#ifdef HARODBG_REGISTERS_HPP
#define HARODBG_REGISTERS_HPP

#include<sys/user.h>
#include<algorithm>

namespace harodbg{
	enum class reg{
		rax, rbx, rcx, rdx,
        rdi, rsi, rbp, rsp,
        r8,  r9,  r10, r11,
        r12, r13, r14, r15,
        rip, rflags,    cs,
        orig_rax, fs_base,
        gs_base,
        fs, gs, ss, ds, es
	};
	
	static constexpr std::size_t n_registers = 27;

}



#endif