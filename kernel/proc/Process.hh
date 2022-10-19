#pragma once

#include <kernel/Handle.hh>
#include <kernel/SpinLock.hh>
#include <kernel/SysResult.hh>
#include <kernel/api/Types.h>
#include <kernel/mem/VirtSpace.hh> // IWYU pragma: keep
#include <kernel/proc/ThreadPriority.hh>
#include <ustd/Atomic.hh>
#include <ustd/Optional.hh> // IWYU pragma: keep
#include <ustd/Shareable.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/String.hh>
#include <ustd/Try.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>
#include <ustd/Vector.hh>

namespace kernel {

struct Scheduler;
class Thread;

class Process : public ustd::Shareable<Process> {
    friend Scheduler;
    friend Thread;

private:
    const size_t m_pid;
    const bool m_is_kernel;
    const ustd::String m_name;
    ustd::SharedPtr<VirtSpace> m_virt_space;
    ustd::Atomic<size_t> m_thread_count{0};

    ustd::Vector<ustd::UniquePtr<HandleBase>, ub_handle_t> m_handle_table;
    SpinLock m_handle_table_lock;

    Process(bool is_kernel, ustd::String &&name, ustd::SharedPtr<VirtSpace> virt_space);

    ub_handle_t allocate_handle(ustd::UniquePtr<HandleBase> &&);
    SysResult<> close_handle(ub_handle_t);
    SysResult<HandleBase &> lookup_handle(ub_handle_t);

public:
    static ustd::SharedPtr<Process> from_pid(size_t pid);

    Process(const Process &) = delete;
    Process(Process &&) = delete;
    ~Process() = default;

    Process &operator=(const Process &) = delete;
    Process &operator=(Process &&) = delete;

    template <typename T>
    ub_handle_t allocate_handle(ustd::SharedPtr<T> object);
    ustd::UniquePtr<Thread> create_thread(ThreadPriority priority);
    template <typename T>
    SysResult<ustd::SharedPtr<T>> lookup_handle(ub_handle_t index);

    size_t pid() const { return m_pid; }
    bool is_kernel() const { return m_is_kernel; }
    ustd::StringView name() const { return m_name; }
    VirtSpace &virt_space() const { return *m_virt_space; }
    size_t thread_count() const { return m_thread_count.load(); }
};

template <typename T>
ub_handle_t Process::allocate_handle(ustd::SharedPtr<T> object) {
    auto handle = ustd::make_unique<Handle<T>>(ustd::move(object));
    return allocate_handle(ustd::move(handle));
}

template <typename T>
SysResult<ustd::SharedPtr<T>> Process::lookup_handle(ub_handle_t index) {
    // TODO: Do we need to keep hold of the lock here for if the UniquePtr<HandleBase> gets destroyed?
    auto &handle = TRY(lookup_handle(index));
    if (handle.kind() != T::k_handle_kind) {
        return Error::BadHandle;
    }
    return static_cast<Handle<T> *>(&handle)->object();
}

} // namespace kernel
