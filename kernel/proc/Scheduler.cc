#include <kernel/proc/Scheduler.hh>

#include <kernel/Port.hh>
#include <kernel/cpu/LocalApic.hh>
#include <kernel/cpu/Processor.hh>
#include <kernel/cpu/RegisterState.hh>
#include <kernel/intr/InterruptManager.hh>
#include <kernel/proc/Process.hh>
#include <ustd/Assert.hh>
#include <ustd/Log.hh>
#include <ustd/Memory.hh>
#include <ustd/Types.hh>

Process *g_current_process = nullptr;

namespace {

constexpr uint16 k_scheduler_frequency = 250;

constexpr uint8 k_pit_vector = 39;
constexpr uint8 k_timer_vector = 40;

LocalApic *s_apic = nullptr;
usize s_ticks = 0;

Process *s_base_process = nullptr;
bool s_pit_fired = false;

void pit_handler(RegisterState *) {
    s_apic->send_eoi();
    s_apic->set_timer(LocalApic::TimerMode::Disabled, 255);
    s_pit_fired = true;
}

void calibrate_timer() {
    // Configure PIT interrupts.
    InterruptManager::wire_interrupt(0, k_pit_vector);
    Processor::wire_interrupt(k_pit_vector, &pit_handler);

    // Initialise PIT to one shot mode for channel 0.
    port_write<uint8>(0x43, 0b00110010);

    // Start the APIC timer counting down from its max value.
    s_apic->set_timer(LocalApic::TimerMode::OneShot, 255);
    s_apic->set_timer_count(0xffffffff);

    // Calculate the divisor based on what frequency we want and send it to the PIT in two parts.
    constexpr uint16 divisor = 1193182ul / k_scheduler_frequency;
    port_write<uint8>(0x40, divisor & 0xffu);
    port_write<uint8>(0x40, (divisor >> 8u) & 0xffu);

    // Enable interrupts and wait for PIT to fire.
    asm volatile("sti");
    while (!s_pit_fired) {
        asm volatile("hlt");
    }
    asm volatile("cli");

    // Work out how many ticks have passed.
    s_ticks = 0xffffffff - s_apic->read_timer_count();

    // TODO: We should probably remask the PIT interrupt here.
}

void handle_page_fault(RegisterState *regs) {
    if ((regs->cs & 3u) == 0u) {
        ENSURE_NOT_REACHED("Page fault in ring 0!");
    }
    logln("[#{}]: Page fault at {:h}!", g_current_process->pid(), regs->rip);
    g_current_process->kill();
    Scheduler::switch_next(regs);
}

} // namespace

void Scheduler::initialise(LocalApic *apic) {
    s_apic = apic;
    calibrate_timer();

    // Initialise idle process.
    s_base_process = Process::create_kernel();
    g_current_process = s_base_process;
    g_current_process->m_next = g_current_process;

    // Initialise the APIC timer in one shot mode so that after each process switch, we can set a new count value
    // depending on the process' priority.
    apic->set_timer(LocalApic::TimerMode::OneShot, k_timer_vector);
    apic->set_timer_count(s_ticks);
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
    g_current_process->m_virt_space.switch_to();
    s_apic->send_eoi();
    s_apic->set_timer_count(s_ticks);
}
