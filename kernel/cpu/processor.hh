#pragma once

#include <ustd/types.hh>

namespace kernel::acpi {

class ApicTable;

} // namespace kernel::acpi

namespace kernel {

class LocalApic;
struct RegisterState;
class Thread;
class VirtSpace;

using InterruptHandler = void (*)(RegisterState *);

struct Processor {
    static void initialise();
    static void setup(uint8_t id);
    static void start_aps(acpi::ApicTable *madt);
    static void set_apic(LocalApic *apic);
    static void set_current_space(VirtSpace *space);
    static void set_current_thread(Thread *thread);
    static void set_kernel_stack(void *stack);
    static void wire_interrupt(uint64_t vector, InterruptHandler handler);
    static void write_cr3(void *pml4);

    static LocalApic *apic();
    static VirtSpace *current_space();
    static Thread *current_thread();
    static uint8_t id();
    static uint8_t *simd_default_region();
    static uint32_t simd_region_size();
};

} // namespace kernel
