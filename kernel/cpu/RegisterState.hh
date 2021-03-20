#pragma once

#include <ustd/Types.hh>

struct [[gnu::packed]] RegisterState {
    uint64 r15, r14, r13, r12, r11, r10, r9, r8, rbp, rsi, rdi, rdx, rcx, rbx, rax;
    uint64 int_num, error_code;
    uint64 rip, cs, rflags, rsp, ss;
};
