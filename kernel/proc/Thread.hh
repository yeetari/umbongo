#pragma once

#include <kernel/SysResult.hh>
#include <kernel/cpu/RegisterState.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/String.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

class Process;
struct Scheduler;

enum class ThreadState {
    Alive,
    Dead,
};

class Thread {
    friend Process;
    friend Scheduler;

private:
    const SharedPtr<Process> m_process;
    RegisterState m_register_state{};
    ThreadState m_state{ThreadState::Alive};

    Thread *m_prev{nullptr};
    Thread *m_next{nullptr};

public:
    template <typename F>
    static Thread *create_kernel(F function) {
        return create_kernel(reinterpret_cast<uintptr>(function));
    }
    static Thread *create_kernel(uintptr entry_point);
    static Thread *create_user();

    explicit Thread(Process *process);
    Thread(const Thread &) = delete;
    Thread(Thread &&) = delete;
    ~Thread();

    Thread &operator=(const Thread &) = delete;
    Thread &operator=(Thread &&) = delete;

    SysResult exec(StringView path, const Vector<String> &args = {});
    void kill();

    Process &process() const { return *m_process; }
    RegisterState &register_state() { return m_register_state; }
};
