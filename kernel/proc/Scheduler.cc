#include <kernel/proc/Scheduler.hh>

#include <kernel/acpi/GenericAddress.hh>
#include <kernel/acpi/HpetTable.hh>
#include <kernel/cpu/LocalApic.hh>
#include <kernel/cpu/Processor.hh>
#include <kernel/cpu/RegisterState.hh>
#include <kernel/mem/VirtSpace.hh>
#include <kernel/proc/Hpet.hh>
#include <kernel/proc/Process.hh>
#include <ustd/Assert.hh>
#include <ustd/Log.hh>
#include <ustd/Memory.hh>
#include <ustd/Types.hh>

Process *g_current_process = nullptr;

namespace {

constexpr uint16 k_scheduler_frequency = 250;
constexpr uint8 k_timer_vector = 40;

Process *s_base_process = nullptr;
Hpet *s_hpet = nullptr;
uint32 s_ticks = 0;

void handle_page_fault(RegisterState *regs) {
    uint64 cr2 = 0;
    asm volatile("mov %%cr2, %0" : "=r"(cr2));
    if ((regs->cs & 3u) == 0u) {
        logln("Page fault at {:h} caused by instruction at {:h}!", cr2, regs->rip);
        ENSURE_NOT_REACHED("Page fault in ring 0!");
    }
    logln("[#{}]: Page fault at {:h} caused by instruction at {:h}!", g_current_process->pid(), cr2, regs->rip);
    g_current_process->kill();
    Scheduler::switch_next(regs);
}

} // namespace

void Scheduler::initialise(acpi::HpetTable *hpet_table) {
    // Retrieve HPET address from ACPI tables and make sure the values are sane.
    const auto &hpet_address = hpet_table->base_address();
    ENSURE(hpet_address.address_space == acpi::AddressSpace::SystemMemory);
    ENSURE(hpet_address.register_bit_offset == 0);

    // Allocate the HPET and enable the main counter.
    s_hpet = new Hpet(hpet_address.address);
    s_hpet->enable();

    // Start the APIC timer counting down from its max value.
    Processor::apic()->set_timer(LocalApic::TimerMode::OneShot, 255);
    Processor::apic()->set_timer_count(0xffffffff);

    // Spin for 100ms and then calculate the total number of ticks occured in 100ms by the APIC timer.
    s_hpet->spin(100);
    s_ticks = 0xffffffff - Processor::apic()->read_timer_count();

    // Calculate the number of ticks for our desired frequency.
    s_ticks /= k_scheduler_frequency;
    s_ticks *= 10;

    // Initialise idle process.
    s_base_process = Process::create_kernel();
    g_current_process = s_base_process;
    g_current_process->m_next = g_current_process;

    // Initialise the APIC timer in one shot mode so that after each process switch, we can set a new count value
    // depending on the process' priority.
    Processor::apic()->set_timer(LocalApic::TimerMode::Periodic, k_timer_vector);
    Processor::apic()->set_timer_count(s_ticks);
    Processor::wire_interrupt(14, &handle_page_fault);
    Processor::wire_interrupt(k_timer_vector, &switch_next);
}

void Scheduler::insert_process(Process *process) {
    process->m_next = s_base_process->m_next;
    s_base_process->m_next = process;
}

void Scheduler::start() {
    asm volatile("sti");
    while (true) {
        asm volatile("hlt");
    }
}

void Scheduler::switch_next(RegisterState *regs) {
    memcpy(&g_current_process->m_register_state, regs, sizeof(RegisterState));
    do {
        g_current_process = g_current_process->m_next;
        ASSERT_PEDANTIC(g_current_process != nullptr);
    } while (g_current_process->m_state != ProcessState::Alive);
    memcpy(regs, &g_current_process->m_register_state, sizeof(RegisterState));
    g_current_process->m_virt_space->switch_to();
    Processor::apic()->set_timer_count(s_ticks);
}

void Scheduler::wait(usize millis) {
    // TODO: Yield instead of spinning. Currently, the code that calls this function is the USB code, which is in
    //       another thread and can potentially yield.
    ASSERT_PEDANTIC(s_hpet != nullptr);
    s_hpet->spin(millis);
}
