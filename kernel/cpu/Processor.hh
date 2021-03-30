#pragma once

#include <kernel/cpu/RegisterState.hh>
#include <ustd/Types.hh>

class LocalApic;

using InterruptHandler = void (*)(RegisterState *);

struct Processor {
    static void initialise();
    static void set_apic(LocalApic *apic);
    static void wire_interrupt(uint64 vector, InterruptHandler handler);
    static void write_cr3(void *pml4);

    static LocalApic *apic();
};
