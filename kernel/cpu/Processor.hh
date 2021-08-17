#pragma once

#include <ustd/Types.hh>

class LocalApic;
struct RegisterState;
class Thread;

using InterruptHandler = void (*)(RegisterState *);

struct Processor {
    static void initialise();
    static void set_apic(LocalApic *apic);
    static void set_current_thread(Thread *thread);
    static void set_kernel_stack(void *stack);
    static void wire_interrupt(uint64 vector, InterruptHandler handler);
    static void write_cr3(void *pml4);

    static LocalApic *apic();
    static Thread *current_thread();
};
