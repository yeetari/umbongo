#include <kernel/proc/Scheduler.hh>

#include <kernel/acpi/GenericAddress.hh>
#include <kernel/acpi/HpetTable.hh>
#include <kernel/cpu/LocalApic.hh>
#include <kernel/cpu/Processor.hh>
#include <kernel/cpu/RegisterState.hh>
#include <kernel/mem/MemoryManager.hh>
#include <kernel/proc/Hpet.hh>
#include <kernel/proc/Process.hh>
#include <kernel/proc/Thread.hh>
#include <kernel/proc/ThreadBlocker.hh>
#include <ustd/Assert.hh>
#include <ustd/Log.hh>
#include <ustd/Memory.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>

namespace {

constexpr uint16 k_scheduler_frequency = 250;
constexpr uint8 k_timer_vector = 40;

Thread *s_base_thread = nullptr;
Hpet *s_hpet = nullptr;
uint32 s_ticks = 0;

void dump_backtrace(RegisterState *regs) {
    auto *rbp = reinterpret_cast<uint64 *>(regs->rbp);
    while (rbp != nullptr && rbp[1] != 0) {
        logln("{:h}", rbp[1]);
        rbp = reinterpret_cast<uint64 *>(*rbp);
    }
}

void handle_fault(RegisterState *regs) {
    auto *thread = Processor::current_thread();
    auto &process = thread->process();
    if ((regs->cs & 3u) == 0u) {
        logln("Fault {} caused by instruction at {:h}!", regs->int_num, regs->rip);
        dump_backtrace(regs);
        ENSURE_NOT_REACHED("Fault in ring 0!");
    }
    logln("[#{}]: Fault {} caused by instruction at {:h}!", process.pid(), regs->int_num, regs->rip);
    dump_backtrace(regs);
    thread->kill();
    Scheduler::switch_next(regs);
}

void handle_page_fault(RegisterState *regs) {
    auto *thread = Processor::current_thread();
    auto &process = thread->process();
    uint64 cr2 = 0;
    asm volatile("mov %%cr2, %0" : "=r"(cr2));
    if ((regs->cs & 3u) == 0u) {
        logln("Page fault at {:h} caused by instruction at {:h}!", cr2, regs->rip);
        dump_backtrace(regs);
        ENSURE_NOT_REACHED("Page fault in ring 0!");
    }
    logln("[#{}]: Page fault at {:h} caused by instruction at {:h}!", process.pid(), cr2, regs->rip);
    dump_backtrace(regs);
    thread->kill();
    Scheduler::switch_next(regs);
}

} // namespace

extern "C" void switch_now(RegisterState *regs);

SharedPtr<Process> Process::from_pid(usize pid) {
    Thread *thread = s_base_thread;
    do {
        if (thread->process().pid() == pid) {
            return thread->m_process;
        }
        thread = thread->m_next;
    } while (thread != s_base_thread);
    return {};
}

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

    // Initialise idle thread and process. We pass null as the entry point as when the thread first gets interrupted,
    // its state will be saved.
    s_base_thread = Thread::create_kernel(nullptr).disown();
    s_base_thread->m_prev = s_base_thread;
    s_base_thread->m_next = s_base_thread;
    Processor::set_current_thread(s_base_thread);

    // Initialise the APIC timer in one shot mode so that after each process switch, we can set a new count value
    // depending on the process' priority.
    Processor::apic()->set_timer(LocalApic::TimerMode::Periodic, k_timer_vector);
    Processor::apic()->set_timer_count(s_ticks);
    Processor::wire_interrupt(0, &handle_fault);
    Processor::wire_interrupt(6, &handle_fault);
    Processor::wire_interrupt(13, &handle_fault);
    Processor::wire_interrupt(14, &handle_page_fault);
    Processor::wire_interrupt(k_timer_vector, &timer_handler);
}

void Scheduler::insert_thread(UniquePtr<Thread> &&unique_thread) {
    auto *thread = unique_thread.disown();
    auto *prev = s_base_thread->m_prev;
    thread->m_prev = prev;
    thread->m_next = s_base_thread;
    s_base_thread->m_prev = thread;
    prev->m_next = thread;
}

void Scheduler::start() {
    asm volatile("sti");
    while (true) {
        auto *thread = s_base_thread;
        do {
            auto *next = thread->m_next;
            if (thread->m_state == ThreadState::Dead) {
                delete thread;
            }
            thread = next;
        } while (thread != s_base_thread);
        asm volatile("hlt");
        Scheduler::yield(true);
    }
}

void Scheduler::switch_next(RegisterState *regs) {
    auto *next_thread = Processor::current_thread();
    do {
        next_thread = next_thread->m_next;
        ASSERT_PEDANTIC(next_thread != nullptr);
        if (next_thread->m_state == ThreadState::Blocked) {
            ASSERT_PEDANTIC(next_thread->m_blocker);
            if (next_thread->m_blocker->should_unblock()) {
                next_thread->m_blocker.clear();
                next_thread->m_state = ThreadState::Alive;
            }
        }
    } while (next_thread->m_state != ThreadState::Alive);
    memcpy(regs, &next_thread->m_register_state, sizeof(RegisterState));
    auto &process = *next_thread->m_process;
    if (MemoryManager::current_space() != process.m_virt_space.obj()) {
        MemoryManager::switch_space(*process.m_virt_space);
    }
    Processor::apic()->set_timer_count(s_ticks);
    Processor::set_current_thread(next_thread);
    Processor::set_kernel_stack(next_thread->m_kernel_stack);
}

void Scheduler::timer_handler(RegisterState *regs) {
    memcpy(&Processor::current_thread()->m_register_state, regs, sizeof(RegisterState));
    switch_next(regs);
}

void Scheduler::wait(usize millis) {
    // TODO: Yield instead of spinning. Currently, the code that calls this function is the USB code, which is in
    //       another thread and can potentially yield.
    ASSERT_PEDANTIC(s_hpet != nullptr);
    s_hpet->spin(millis);
}

void Scheduler::yield(bool save_state) {
    if (save_state) {
        asm volatile("int $40");
        return;
    }
    RegisterState regs{};
    switch_next(&regs);
    switch_now(&regs);
}

void Scheduler::yield_and_kill() {
    Processor::current_thread()->kill();
    yield(false);
}
