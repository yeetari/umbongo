#include <kernel/proc/Scheduler.hh>

#include <kernel/Dmesg.hh>
#include <kernel/ScopedLock.hh>
#include <kernel/SpinLock.hh>
#include <kernel/cpu/LocalApic.hh>
#include <kernel/cpu/Processor.hh>
#include <kernel/cpu/RegisterState.hh>
#include <kernel/mem/MemoryManager.hh>
#include <kernel/proc/Process.hh>
#include <kernel/proc/Thread.hh>
#include <kernel/proc/ThreadPriority.hh>
#include <kernel/time/TimeManager.hh>
#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/Atomic.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>
#include <ustd/Utility.hh>

namespace kernel {
namespace {

class PriorityQueue {
    static constexpr uint32 k_slot_count = 1024;
    alignas(64) ustd::Array<ustd::Atomic<Thread *>, k_slot_count> m_slots;
    alignas(64) ustd::Atomic<uint32> m_head;
    alignas(64) ustd::Atomic<uint32> m_tail;

public:
    void enqueue(Thread *thread);
    Thread *dequeue();
};

void PriorityQueue::enqueue(Thread *thread) {
    uint32 head = m_head.fetch_add(1, ustd::MemoryOrder::Acquire);
    auto &slot = m_slots[head % k_slot_count];
    while (!slot.compare_exchange_temp(nullptr, thread, ustd::MemoryOrder::Release, ustd::MemoryOrder::Relaxed)) {
        do {
            asm volatile("pause");
        } while (slot.load(ustd::MemoryOrder::Relaxed) != nullptr);
    }
}

Thread *PriorityQueue::dequeue() {
    uint32 tail = m_tail.load(ustd::MemoryOrder::Relaxed);
    do {
        if (static_cast<int32>(m_head.load(ustd::MemoryOrder::Relaxed) - tail) <= 0) {
            return nullptr;
        }
    } while (!m_tail.compare_exchange(tail, tail + 1, ustd::MemoryOrder::Acquire, ustd::MemoryOrder::Relaxed));
    auto &slot = m_slots[tail % k_slot_count];
    while (true) {
        Thread *thread = slot.exchange(nullptr, ustd::MemoryOrder::Release);
        if (thread != nullptr) {
            return thread;
        }
        do {
            asm volatile("pause");
        } while (slot.load(ustd::MemoryOrder::Relaxed) == nullptr);
    }
}

constexpr uint8 k_timer_vector = 40;
uint32 s_ticks_in_one_ms = 0;

Thread *s_base_thread;
SpinLock s_thread_list_lock;

PriorityQueue *s_blocked_queue;
ustd::Array<PriorityQueue *, 2> s_queues;

void dump_backtrace(RegisterState *regs) {
    auto *rbp = reinterpret_cast<uint64 *>(regs->rbp);
    while (rbp != nullptr && rbp[1] != 0) {
        dmesg("{:h}", rbp[1]);
        rbp = reinterpret_cast<uint64 *>(*rbp);
    }
}

void handle_fault(RegisterState *regs) {
    auto *thread = Processor::current_thread();
    auto &process = thread->process();
    if ((regs->cs & 3u) == 0u) {
        dmesg_unlock();
        dmesg("Fault {} caused by instruction at {:h}!", regs->int_num, regs->rip);
        dump_backtrace(regs);
        ENSURE_NOT_REACHED("Fault in ring 0!");
    }
    dmesg("[#{}]: Fault {} caused by instruction at {:h}!", process.pid(), regs->int_num, regs->rip);
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
        dmesg_unlock();
        dmesg("Page fault at {:h} caused by instruction at {:h}!", cr2, regs->rip);
        dump_backtrace(regs);
        ENSURE_NOT_REACHED("Page fault in ring 0!");
    }
    dmesg("[#{}]: Page fault at {:h} caused by instruction at {:h}!", process.pid(), cr2, regs->rip);
    dump_backtrace(regs);
    thread->kill();
    Scheduler::switch_next(regs);
}

Thread *pick_next() {
    while (auto *thread = s_blocked_queue->dequeue()) {
        s_queues[static_cast<uint32>(thread->priority())]->enqueue(thread);
    }
    for (auto *queue = s_queues.end(); queue-- != s_queues.begin();) {
        while (auto *thread = (*queue)->dequeue()) {
            thread->try_unblock();
            if (thread->state() == ThreadState::Alive) {
                return thread;
            }
            s_blocked_queue->enqueue(thread);
        }
    }
    ENSURE_NOT_REACHED("Thread queue empty!");
}

uint32 time_slice_for(Thread *thread) {
    return (static_cast<uint32>(thread->priority()) + 1u) * s_ticks_in_one_ms;
}

} // namespace

extern "C" void switch_now(RegisterState *regs);

ustd::SharedPtr<Process> Process::from_pid(usize pid) {
    ScopedLock locker(s_thread_list_lock);
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

    // Spin for 1 ms and then calculate the total number of ticks occured in that time by the APIC timer.
    TimeManager::spin(1);
    s_ticks_in_one_ms = 0xffffffff - Processor::apic()->read_timer_count();

    // Initialise idle thread and process. We pass null as the entry point as when the thread first gets interrupted,
    // its state will be saved.
    s_base_thread = Thread::create_kernel(nullptr, ThreadPriority::Idle).disown();
    s_base_thread->m_prev = s_base_thread;
    s_base_thread->m_next = s_base_thread;
    Processor::set_current_thread(s_base_thread);

    // Wire various interrupts for fault handling and timing.
    Processor::wire_interrupt(0, &handle_fault);
    Processor::wire_interrupt(6, &handle_fault);
    Processor::wire_interrupt(13, &handle_fault);
    Processor::wire_interrupt(14, &handle_page_fault);
    Processor::wire_interrupt(k_timer_vector, &timer_handler);

    s_blocked_queue = new PriorityQueue;
    for (auto &queue : s_queues) {
        queue = new PriorityQueue;
    }
}

void Scheduler::insert_thread(ustd::UniquePtr<Thread> &&unique_thread) {
    ScopedLock locker(s_thread_list_lock);
    auto *thread = unique_thread.disown();
    thread->m_prev = s_base_thread->m_prev;
    thread->m_next = s_base_thread;
    thread->m_prev->m_next = thread;
    thread->m_next->m_prev = thread;
    if (thread->priority() != ThreadPriority::Idle) {
        s_queues[static_cast<uint32>(thread->priority())]->enqueue(thread);
    }
}

void Scheduler::setup() {
    // Enable the local APIC and acknowledge any outstanding interrupts.
    Processor::apic()->enable();
    Processor::apic()->send_eoi();

    // Initialise the APIC timer in one shot mode so that after each process switch, we can set a new count value
    // depending on the process' priority.
    Processor::apic()->set_timer(LocalApic::TimerMode::OneShot, k_timer_vector);
    Processor::apic()->set_timer_count(s_ticks_in_one_ms);

    // Each AP needs an idle thread created to ensure that there is always something to schedule.
    if (Processor::id() != 0) {
        auto idle_thread = Thread::create_kernel(&start, ThreadPriority::Idle);
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
    Thread *current_thread = Processor::current_thread();
    if (current_thread->m_state != ThreadState::Dead) {
        s_queues[static_cast<uint32>(current_thread->priority())]->enqueue(current_thread);
    } else {
        delete current_thread;
    }

    Thread *next_thread = pick_next();
    __builtin_memcpy(regs, &next_thread->m_register_state, sizeof(RegisterState));
    auto &process = *next_thread->m_process;
    if (MemoryManager::current_space() != process.m_virt_space.obj()) {
        MemoryManager::switch_space(*process.m_virt_space);
    }
    Processor::apic()->set_timer_count(time_slice_for(next_thread));
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

} // namespace kernel
