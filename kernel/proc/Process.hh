#pragma once

#include <kernel/SpinLock.hh>
#include <kernel/SysResult.hh>
#include <kernel/api/Types.hh> // IWYU pragma: keep
#include <kernel/fs/FileHandle.hh>
#include <kernel/mem/VirtSpace.hh> // IWYU pragma: keep
#include <ustd/Atomic.hh>
#include <ustd/Optional.hh>
#include <ustd/Shareable.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>
#include <ustd/Vector.hh>

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
    ~Process() = default;

    Process &operator=(const Process &) = delete;
    Process &operator=(Process &&) = delete;

    ustd::UniquePtr<Thread> create_thread(ThreadPriority priority);

#define S(name, ...) SyscallResult sys_##name(__VA_ARGS__);
#include <kernel/api/Syscalls.in>
#undef S

    size_t pid() const { return m_pid; }
    bool is_kernel() const { return m_is_kernel; }
    FileHandle &file_handle(uint32_t fd) { return *m_fds[fd]; }
    size_t thread_count() const { return m_thread_count.load(ustd::MemoryOrder::Relaxed); }
};

} // namespace kernel
