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

    SyscallResult sys_accept(uint32_t fd);
    SyscallResult sys_allocate_region(size_t size, MemoryProt prot);
    SyscallResult sys_bind(uint32_t fd, const char *path);
    SyscallResult sys_chdir(const char *path);
    SyscallResult sys_close(uint32_t fd);
    SyscallResult sys_connect(const char *path);
    SyscallResult sys_create_pipe(uint32_t *fds);
    SyscallResult sys_create_process(const char *path, const char **argv, FdPair *copy_fds);
    SyscallResult sys_create_server_socket(uint32_t backlog_limit);
    SyscallResult sys_debug_line(const char *line);
    SyscallResult sys_dup_fd(uint32_t src, uint32_t dst);
    SyscallResult sys_exit(size_t code) const;
    SyscallResult sys_getcwd(char *path) const;
    SyscallResult sys_getpid() const;
    SyscallResult sys_gettime() const;
    SyscallResult sys_ioctl(uint32_t fd, IoctlRequest request, void *arg);
    SyscallResult sys_mkdir(const char *path) const;
    SyscallResult sys_mmap(uint32_t fd) const;
    SyscallResult sys_mount(const char *target, const char *fs_type) const;
    SyscallResult sys_open(const char *path, OpenMode mode);
    SyscallResult sys_poll(PollFd *fds, size_t count, ssize_t timeout);
    SyscallResult sys_read(uint32_t fd, void *data, size_t size);
    SyscallResult sys_read_directory(const char *path, uint8_t *data);
    SyscallResult sys_seek(uint32_t fd, size_t offset, SeekMode mode);
    SyscallResult sys_size(uint32_t fd);
    SyscallResult sys_virt_to_phys(uintptr_t virt);
    SyscallResult sys_wait_pid(size_t pid);
    SyscallResult sys_write(uint32_t fd, void *data, size_t size);

    size_t pid() const { return m_pid; }
    bool is_kernel() const { return m_is_kernel; }
    FileHandle &file_handle(uint32_t fd) { return *m_fds[fd]; }
    size_t thread_count() const { return m_thread_count.load(ustd::MemoryOrder::Relaxed); }
};

} // namespace kernel
