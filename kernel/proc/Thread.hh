#pragma once

#include <kernel/SysResult.hh>
#include <kernel/cpu/RegisterState.hh>
#include <kernel/proc/Scheduler.hh>
#include <ustd/Assert.hh>
#include <ustd/Atomic.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/String.hh> // IWYU pragma: keep
#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>
#include <ustd/Utility.hh>
#include <ustd/Vector.hh>

namespace kernel {

class Process;
class ThreadBlocker;

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
    ustd::UniquePtr<ThreadBlocker> m_blocker;
    ustd::Atomic<int16> m_cpu{-1};
    uint8 *m_kernel_stack{nullptr};

    Thread *m_prev{nullptr};
    Thread *m_next{nullptr};

public:
    template <typename F>
    static ustd::UniquePtr<Thread> create_kernel(F function) {
        return create_kernel(reinterpret_cast<uintptr>(function));
    }
    static ustd::UniquePtr<Thread> create_kernel(uintptr entry_point);
    static ustd::UniquePtr<Thread> create_user();

    explicit Thread(Process *process);
    Thread(const Thread &) = delete;
    Thread(Thread &&) = delete;
    ~Thread();

    Thread &operator=(const Thread &) = delete;
    Thread &operator=(Thread &&) = delete;

    template <typename T, typename... Args>
    void block(Args &&...args);
    SysResult<> exec(ustd::StringView path, const ustd::Vector<ustd::String> &args = {});
    void kill();

    Process &process() const { return *m_process; }
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
