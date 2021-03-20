#pragma once

#include <kernel/cpu/RegisterState.hh>
#include <ustd/Types.hh>

using InterruptHandler = void (*)(RegisterState *);

struct Processor {
    static void initialise();
    static void wire_interrupt(uint64 vector, InterruptHandler handler);
    static void write_cr3(void *pml4);
};
