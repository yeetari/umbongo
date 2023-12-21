#pragma once

#include <ustd/types.hh>

namespace kernel {

struct [[gnu::packed]] RegisterState {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8, rbp, rsi, rdi, rdx, rcx, rbx, rax;
    uint64_t int_num, error_code;
    uint64_t rip, cs, rflags, rsp, ss;
};

} // namespace kernel
