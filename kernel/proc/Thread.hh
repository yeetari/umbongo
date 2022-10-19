#pragma once

#include <kernel/SysResult.hh>
#include <kernel/UserPtr.hh>
#include <kernel/api/Types.h>
#include <kernel/cpu/RegisterState.hh>
#include <kernel/proc/ThreadPriority.hh>
#include <ustd/Assert.hh>
#include <ustd/Atomic.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/String.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>
#include <ustd/Utility.hh>
#include <ustd/Vector.hh>

namespace kernel {

class Process;
class Region;
struct Scheduler;

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
    const ThreadPriority m_priority;
    ustd::Atomic<ThreadState> m_state;
    RegisterState m_register_state{};
    uint8_t *m_kernel_stack{nullptr};
    uint8_t *m_simd_region{nullptr};
    ustd::Vector<ustd::SharedPtr<Region>> m_executable_regions;

    Thread *m_prev{nullptr};
    Thread *m_next{nullptr};

    bool copy_from_user(void *dst, uintptr_t src, size_t size);
    bool copy_to_user(uintptr_t dst, void *src, size_t size);

public:
    template <typename F>
    static ustd::UniquePtr<Thread> create_kernel(F function, ThreadPriority priority, ustd::String &&name) {
        return create_kernel(reinterpret_cast<uintptr_t>(function), priority, ustd::move(name));
    }
    static ustd::UniquePtr<Thread> create_kernel(uintptr_t entry_point, ThreadPriority priority, ustd::String &&name);
    static ustd::UniquePtr<Thread> create_user(ThreadPriority priority, ustd::String &&name);
    static void kill_and_yield();

    Thread(Process *process, ThreadPriority priority);
    Thread(const Thread &) = delete;
    Thread(Thread &&) = delete;
    ~Thread();

    Thread &operator=(const Thread &) = delete;
    Thread &operator=(Thread &&) = delete;

    template <typename T>
    SysResult<> copy_from_user(void *dst, UserPtr<T> src, size_t count = 1);
    template <typename T>
    SysResult<> copy_to_user(UserPtr<T> dst, void *src, size_t count = 1);
    SysResult<> exec(UserPtr<const uint8_t> binary, size_t binary_size);
    void handle_fault(RegisterState *regs);
    void set_state(ThreadState state);
    void unblock();

#define S(name, ...) SyscallResult sys_##name(__VA_ARGS__);
#include <kernel/api/Syscalls.in>
#undef S

    Process &process() const { return *m_process; }
    ThreadPriority priority() const { return m_priority; }
    RegisterState &register_state() { return m_register_state; }
    ThreadState state() const { return m_state.load(); }
};

template <typename T>
SysResult<> Thread::copy_from_user(void *dst, UserPtr<T> src, size_t count) {
    return copy_from_user(dst, src.ptr_unsafe(), count * sizeof(T)) ? Error::BadAddress : SysResult<>{};
}

template <typename T>
SysResult<> Thread::copy_to_user(UserPtr<T> dst, void *src, size_t count) {
    return copy_to_user(dst.ptr_unsafe(), src, count * sizeof(T)) ? Error::BadAddress : SysResult<>{};
}

} // namespace kernel
