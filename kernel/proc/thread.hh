#pragma once

#include <kernel/cpu/register_state.hh>
#include <kernel/proc/scheduler.hh>
#include <kernel/sys_result.hh>
#include <ustd/assert.hh>
#include <ustd/shared_ptr.hh>
#include <ustd/string.hh> // IWYU pragma: keep
#include <ustd/string_view.hh>
#include <ustd/types.hh>
#include <ustd/unique_ptr.hh>
#include <ustd/utility.hh>
#include <ustd/vector.hh>

namespace kernel {

class Process;
class ThreadBlocker;

enum class ThreadPriority : uint32_t {
    Idle = 0,
    Normal = 1,
};

enum class ThreadState {
    Alive,
    Blocked,
    Dead,
};

class Thread {
    friend Process;
    friend Scheduler;

private:
    const ustd::SharedPtr<Process> m_process;
    RegisterState m_register_state{};
    ThreadState m_state{ThreadState::Alive};
    const ThreadPriority m_priority;
    ustd::UniquePtr<ThreadBlocker> m_blocker;
    uint8_t *m_kernel_stack{nullptr};
    uint8_t *m_simd_region{nullptr};

    Thread *m_prev{nullptr};
    Thread *m_next{nullptr};

public:
    template <typename F>
    static ustd::UniquePtr<Thread> create_kernel(F function, ThreadPriority priority) {
        return create_kernel(reinterpret_cast<uintptr_t>(function), priority);
    }
    static ustd::UniquePtr<Thread> create_kernel(uintptr_t entry_point, ThreadPriority priority);
    static ustd::UniquePtr<Thread> create_user(ThreadPriority priority);

    Thread(Process *process, ThreadPriority priority);
    Thread(const Thread &) = delete;
    Thread(Thread &&) = delete;
    ~Thread();

    Thread &operator=(const Thread &) = delete;
    Thread &operator=(Thread &&) = delete;

    template <typename T, typename... Args>
    void block(Args &&...args);
    SysResult<> exec(ustd::StringView path, const ustd::Vector<ustd::String> &args = {});
    void handle_fault(RegisterState *regs);
    void kill();
    void try_unblock();

    Process &process() const { return *m_process; }
    ThreadPriority priority() const { return m_priority; }
    RegisterState &register_state() { return m_register_state; }
    ThreadState state() const { return m_state; }
};

template <typename T, typename... Args>
void Thread::block(Args &&...args) {
    ASSERT(m_state != ThreadState::Blocked);
    m_blocker = ustd::make_unique<T>(ustd::forward<Args>(args)...);
    m_state = ThreadState::Blocked;
    Scheduler::yield(true);
}

} // namespace kernel
