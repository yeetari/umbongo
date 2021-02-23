#pragma once

#include <ustd/Types.hh>

class MemoryManager;

struct [[gnu::packed]] InterruptFrame {
    uint64 r15, r14, r13, r12, r11, r10, r9, r8, rbp, rsi, rdi, rdx, rcx, rbx, rax;
    uint64 num, error_code;
    uint64 rip, cs, rflags, rsp, ss;
};

using InterruptHandler = void (*)(InterruptFrame *);

struct Processor {
    static void initialise(MemoryManager &memory_manager);
    static void wire_interrupt(uint64 vector, InterruptHandler handler);
    static void write_cr3(void *pml4);
};
