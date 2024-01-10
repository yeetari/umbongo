#pragma once

#include <kernel/arch/arch.hh>
#include <kernel/mem/virtual_range.hh>
#include <ustd/types.hh>

// IWYU pragma: begin_keep
#if ARCH(amd64)
#include <kernel/arch/amd64/cpu.hh>
#else
#error Unhandled architecture
#endif
// IWYU pragma: end_keep

namespace kernel {

class AddressSpace;
class Thread;

} // namespace kernel

namespace kernel::acpi {

class RootTable;

} // namespace kernel::acpi

namespace kernel::arch {

struct RegisterState;

using InterruptHandler = void (*)(RegisterState *);

uint32_t current_cpu();
void bsp_init(const acpi::RootTable *xsdt);
void smp_init(const acpi::RootTable *xsdt);
[[noreturn]] void sched_start(Thread *base_thread);
void switch_space(AddressSpace &address_space);
void thread_init(Thread *thread);
void thread_load(RegisterState *state, Thread *thread);
void thread_save(RegisterState *state);
void timer_set_one_shot(uint32_t ms);
void tlb_flush_range(AddressSpace &address_space, VirtualRange range);
void vm_debug_char(char ch);
void wire_interrupt(uint8_t vector, InterruptHandler handler);
void wire_timer(InterruptHandler handler);

} // namespace kernel::arch
