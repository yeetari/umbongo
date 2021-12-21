#include <kernel/proc/Scheduler.hh>

#include <kernel/ScopedLock.hh>
#include <kernel/SpinLock.hh>
#include <kernel/cpu/LocalApic.hh>
#include <kernel/cpu/Processor.hh>
#include <kernel/cpu/RegisterState.hh>
#include <kernel/mem/MemoryManager.hh>
#include <kernel/proc/Process.hh>
#include <kernel/proc/Thread.hh>
#include <kernel/proc/ThreadBlocker.hh>
#include <kernel/time/TimeManager.hh>
#include <ustd/Assert.hh>
#include <ustd/Atomic.hh>
#include <ustd/Log.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>
#include <ustd/Utility.hh>

namespace {

constexpr uint16 k_scheduler_frequency = 250;
constexpr uint8 k_timer_vector = 40;

Thread *s_base_thread = nullptr;
SpinLock s_scheduler_lock;
uint32 s_ticks = 0;

void dump_backtrace(RegisterState *regs) {
    auto *rbp = reinterpret_cast<uint64 *>(regs->rbp);
    while (rbp != nullptr && rbp[1] != 0) {
        ustd::dbgln("{:h}", rbp[1]);
        rbp = reinterpret_cast<uint64 *>(*rbp);
    }
}

void handle_fault(RegisterState *regs) {
    auto *thread = Processor::current_thread();
    auto &process = thread->process();
    if ((regs->cs & 3u) == 0u) {
        log_unlock();
        ustd::dbgln("Fault {} caused by instruction at {:h}!", regs->int_num, regs->rip);
        dump_backtrace(regs);
        ENSURE_NOT_REACHED("Fault in ring 0!");
    }
    ustd::dbgln("[#{}]: Fault {} caused by instruction at {:h}!", process.pid(), regs->int_num, regs->rip);
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
        log_unlock();
        ustd::dbgln("Page fault at {:h} caused by instruction at {:h}!", cr2, regs->rip);
        dump_backtrace(regs);
        ENSURE_NOT_REACHED("Page fault in ring 0!");
    }
    ustd::dbgln("[#{}]: Page fault at {:h} caused by instruction at {:h}!", process.pid(), cr2, regs->rip);
    dump_backtrace(regs);
    thread->kill();
    Scheduler::switch_next(regs);
}

} // namespace

extern "C" void switch_now(RegisterState *regs);

ustd::SharedPtr<Process> Process::from_pid(usize pid) {
    ScopedLock locker(s_scheduler_lock);
    Thread *thread = s_base_thread;
    do {
        if (thread->process().pid() == pid) {
            return thread->m_process;
        }
        thread = thread->m_next;
    } while (thread != s_base_thread);
    return {};
}

void Scheduler::initialise() {
    // Start the APIC timer counting down from its max value.
    Processor::apic()->set_timer(LocalApic::TimerMode::OneShot, 255);
    Processor::apic()->set_timer_count(0xffffffff);

    // Spin for 100ms and then calculate the total number of ticks occured in 100ms by the APIC timer.
    TimeManager::spin(100);
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

    // Wire various interrupts for fault handling and timing.
    Processor::wire_interrupt(0, &handle_fault);
    Processor::wire_interrupt(6, &handle_fault);
    Processor::wire_interrupt(13, &handle_fault);
    Processor::wire_interrupt(14, &handle_page_fault);
    Processor::wire_interrupt(k_timer_vector, &timer_handler);
}

void Scheduler::insert_thread(ustd::UniquePtr<Thread> &&unique_thread) {
    ScopedLock locker(s_scheduler_lock);
    auto *thread = unique_thread.disown();
    auto *prev = s_base_thread->m_prev;
    thread->m_prev = prev;
    thread->m_next = s_base_thread;
    s_base_thread->m_prev = thread;
    prev->m_next = thread;
}

void Scheduler::setup() {
    // Enable the local APIC and acknowledge any outstanding interrupts.
    Processor::apic()->enable();
    Processor::apic()->send_eoi();

    // Initialise the APIC timer in one shot mode so that after each process switch, we can set a new count value
    // depending on the process' priority.
    Processor::apic()->set_timer(LocalApic::TimerMode::OneShot, k_timer_vector);
    Processor::apic()->set_timer_count(s_ticks);

    // Each AP needs an idle thread created to ensure that there is always something to schedule.
    // TODO: Better idle thread system. Currently, idle threads are scheduled normally, wasting time if there is actual
    //       work to be done.
    if (Processor::id() != 0) {
        auto idle_thread = Thread::create_kernel(&start);
        Processor::set_current_thread(idle_thread.obj());
        insert_thread(ustd::move(idle_thread));
    }
}

void Scheduler::start() {
    asm volatile("sti");
    while (true) {
        asm volatile("hlt");
    }
}

void Scheduler::switch_next(RegisterState *regs) {
    ScopedLock locker(s_scheduler_lock);
    Processor::current_thread()->m_cpu.store(-1, ustd::MemoryOrder::Release);

    auto *next_thread = Processor::current_thread();
    do {
        if (next_thread->m_state == ThreadState::Dead) {
            auto *to_delete = next_thread;
            next_thread = next_thread->m_next;
            delete to_delete;
        } else {
            next_thread = next_thread->m_next;
        }
        ASSERT_PEDANTIC(next_thread != nullptr);
        if (next_thread->m_state == ThreadState::Blocked) {
            ASSERT_PEDANTIC(next_thread->m_blocker);
            if (next_thread->m_blocker->should_unblock()) {
                next_thread->m_blocker.clear();
                next_thread->m_state = ThreadState::Alive;
            }
        }
    } while (next_thread->m_state != ThreadState::Alive || next_thread->m_cpu.load(ustd::MemoryOrder::Acquire) != -1);
    next_thread->m_cpu.store(Processor::id(), ustd::MemoryOrder::Release);
    locker.unlock();
    __builtin_memcpy(regs, &next_thread->m_register_state, sizeof(RegisterState));
    auto &process = *next_thread->m_process;
    if (MemoryManager::current_space() != process.m_virt_space.obj()) {
        MemoryManager::switch_space(*process.m_virt_space);
    }
    Processor::apic()->set_timer_count(s_ticks);
    Processor::set_current_thread(next_thread);
    Processor::set_kernel_stack(next_thread->m_kernel_stack);
}

void Scheduler::timer_handler(RegisterState *regs) {
    if (Processor::id() == 0) {
        TimeManager::update();
    }
    __builtin_memcpy(&Processor::current_thread()->m_register_state, regs, sizeof(RegisterState));
    switch_next(regs);
}

void Scheduler::wait(usize millis) {
    // TODO: Yield instead of spinning. Currently, the code that calls this function is the USB code, which is in
    //       another thread and can potentially yield.
    TimeManager::spin(millis);
}

void Scheduler::yield(bool save_state) {
    if (save_state) {
        asm volatile("int $40");
        return;
    }
    asm volatile("cli");
    RegisterState regs{};
    switch_next(&regs);
    switch_now(&regs);
}

void Scheduler::yield_and_kill() {
    Processor::current_thread()->kill();
    yield(false);
}
