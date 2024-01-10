#pragma once

#include <kernel/fs/file_handle.hh>
#include <kernel/mem/virt_space.hh> // IWYU pragma: keep
#include <kernel/spin_lock.hh>
#include <kernel/sys_result.hh>
#include <ustd/atomic.hh>
#include <ustd/optional.hh>
#include <ustd/shareable.hh>
#include <ustd/shared_ptr.hh>
#include <ustd/types.hh>
#include <ustd/unique_ptr.hh>
#include <ustd/vector.hh>

namespace kernel {

class Inode;
class Thread;
enum class ThreadPriority : uint32_t;
struct Scheduler;

class Process : public ustd::Shareable<Process> {
    friend Scheduler;
    friend Thread;

private:
    const size_t m_pid;
    const bool m_is_kernel;
    Inode *m_cwd;
    ustd::SharedPtr<VirtSpace> m_virt_space;
    ustd::Vector<ustd::Optional<FileHandle>> m_fds;
    ustd::Atomic<size_t> m_thread_count{0};
    mutable SpinLock m_lock;

    Process(bool is_kernel, ustd::SharedPtr<VirtSpace> virt_space);

    uint32_t allocate_fd();

public:
    static ustd::SharedPtr<Process> from_pid(size_t pid);

    Process(const Process &) = delete;
    Process(Process &&) = delete;
    ~Process();

    Process &operator=(const Process &) = delete;
    Process &operator=(Process &&) = delete;

    ustd::UniquePtr<Thread> create_thread(ThreadPriority priority);

#define S(name, ...) SyscallResult sys_##name(__VA_ARGS__);
#include <kernel/api/syscalls.in>
#undef S

    size_t pid() const { return m_pid; }
    bool is_kernel() const { return m_is_kernel; }
    VirtSpace &virt_space() { return *m_virt_space; }
    FileHandle &file_handle(uint32_t fd) { return *m_fds[fd]; }
    size_t thread_count() const { return m_thread_count.load(ustd::memory_order_relaxed); }
};

} // namespace kernel
