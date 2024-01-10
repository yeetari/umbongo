#include <kernel/proc/scheduler.hh>

#include <kernel/arch/cpu.hh>
#include <kernel/arch/register_state.hh>
#include <kernel/mem/memory_manager.hh>
#include <kernel/proc/process.hh>
#include <kernel/proc/thread.hh>
#include <kernel/scoped_lock.hh>
#include <kernel/spin_lock.hh>
#include <kernel/time/time_manager.hh>
#include <ustd/array.hh>
#include <ustd/assert.hh>
#include <ustd/atomic.hh>
#include <ustd/shared_ptr.hh>
#include <ustd/types.hh>
#include <ustd/unique_ptr.hh>
#include <ustd/utility.hh>

namespace kernel {
namespace {

class PriorityQueue {
    static constexpr uint32_t k_slot_count = 1024;
    alignas(64) ustd::Array<ustd::Atomic<Thread *>, k_slot_count> m_slots;
    alignas(64) ustd::Atomic<uint32_t> m_head;
    alignas(64) ustd::Atomic<uint32_t> m_tail;

public:
    void enqueue(Thread *thread);
    Thread *dequeue();
};

void PriorityQueue::enqueue(Thread *thread) {
    uint32_t head = m_head.fetch_add(1, ustd::memory_order_acquire);
    auto &slot = m_slots[head % k_slot_count];
    while (!slot.cmpxchg(nullptr, thread, ustd::memory_order_release)) {
        do {
            arch::cpu_relax();
        } while (slot.load(ustd::memory_order_relaxed) != nullptr);
    }
}

Thread *PriorityQueue::dequeue() {
    uint32_t tail = m_tail.load(ustd::memory_order_relaxed);
    do {
        if (static_cast<int32_t>(m_head.load(ustd::memory_order_relaxed) - tail) <= 0) {
            return nullptr;
        }
    } while (!m_tail.compare_exchange(tail, tail + 1, ustd::memory_order_acquire));
    auto &slot = m_slots[tail % k_slot_count];
    while (true) {
        Thread *thread = slot.exchange(nullptr, ustd::memory_order_release);
        if (thread != nullptr) {
            return thread;
        }
        do {
            arch::cpu_relax();
        } while (slot.load(ustd::memory_order_relaxed) == nullptr);
    }
}

Thread *s_base_thread;
SpinLock s_thread_list_lock;
ustd::Atomic<bool> s_time_being_updated;

PriorityQueue *s_blocked_queue;
ustd::Array<PriorityQueue *, 2> s_queues;

Thread *pick_next() {
    while (auto *thread = s_blocked_queue->dequeue()) {
        s_queues[static_cast<uint32_t>(thread->priority())]->enqueue(thread);
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

uint32_t time_slice_for(Thread *thread) {
    return static_cast<uint32_t>(thread->priority()) + 1u;
}

} // namespace

extern "C" void switch_now(arch::RegisterState *regs);

ustd::SharedPtr<Process> Process::from_pid(size_t pid) {
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
    // Initialise idle thread and process. We pass null as the entry point as when the thread first gets interrupted,
    // its state will be saved.
    s_base_thread = Thread::create_kernel(nullptr, ThreadPriority::Idle).disown();
    s_base_thread->m_prev = s_base_thread;
    s_base_thread->m_next = s_base_thread;

    s_blocked_queue = new PriorityQueue;
    for (auto &queue : s_queues) {
        queue = new PriorityQueue;
    }
}

[[noreturn]] void Scheduler::start_bsp() {
    // Start on the BSP.
    arch::wire_timer(&timer_handler);
    arch::sched_start(s_base_thread);
}

void Scheduler::insert_thread(ustd::UniquePtr<Thread> &&unique_thread) {
    ScopedLock locker(s_thread_list_lock);
    auto *thread = unique_thread.disown();
    thread->m_prev = s_base_thread->m_prev;
    thread->m_next = s_base_thread;
    thread->m_prev->m_next = thread;
    thread->m_next->m_prev = thread;
    if (thread->priority() != ThreadPriority::Idle) {
        s_queues[static_cast<uint32_t>(thread->priority())]->enqueue(thread);
    }
}

void Scheduler::switch_next(arch::RegisterState *regs) {
    // Requeue switched from thread if not dead.
    Thread *current_thread = &Thread::current();
    if (current_thread->m_state != ThreadState::Dead) {
        s_queues[static_cast<uint32_t>(current_thread->priority())]->enqueue(current_thread);
    }

    Thread *next_thread = pick_next();
    arch::switch_space(next_thread->process().address_space());

    // Only delete previous thread after switching address space.
    if (current_thread->m_state == ThreadState::Dead) {
        delete current_thread;
    }

    arch::timer_set_one_shot(time_slice_for(next_thread));
    arch::thread_load(regs, next_thread);
}

void Scheduler::timer_handler(arch::RegisterState *regs) {
    if (s_time_being_updated.cmpxchg(false, true, ustd::memory_order_acq_rel, ustd::memory_order_acquire)) {
        TimeManager::update();
        s_time_being_updated.store(false, ustd::memory_order_release);
    }
    arch::thread_save(regs);
    switch_next(regs);
}

void Scheduler::yield(bool save_state) {
    if (save_state) {
        asm volatile("int $0xec");
        return;
    }
    arch::RegisterState regs{};
    switch_next(&regs);
    switch_now(&regs);
}

void Scheduler::yield_and_kill() {
    Thread::current().kill();
    yield(false);
}

} // namespace kernel
