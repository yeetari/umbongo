#pragma once

#include <kernel/SpinLock.hh>
#include <kernel/SysResult.hh>
#include <kernel/SyscallTypes.hh>
#include <kernel/fs/FileHandle.hh>
#include <kernel/mem/VirtSpace.hh> // IWYU pragma: keep
#include <ustd/Atomic.hh>
#include <ustd/Optional.hh> // IWYU pragma: keep
#include <ustd/Shareable.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>
#include <ustd/Vector.hh>

class Inode;
struct Scheduler;
class Thread;

class Process : public Shareable<Process> {
    friend Scheduler;
    friend Thread;

private:
    const usize m_pid;
    const bool m_is_kernel;
    Inode *m_cwd;
    SharedPtr<VirtSpace> m_virt_space;
    Vector<Optional<FileHandle>> m_fds;
    Atomic<usize> m_thread_count{0};
    mutable SpinLock m_lock;

    Process(bool is_kernel, SharedPtr<VirtSpace> virt_space);

    uint32 allocate_fd();

public:
    static SharedPtr<Process> from_pid(usize pid);

    Process(const Process &) = delete;
    Process(Process &&) = delete;
    ~Process() = default;

    Process &operator=(const Process &) = delete;
    Process &operator=(Process &&) = delete;

    UniquePtr<Thread> create_thread();

    SyscallResult sys_accept(uint32 fd);
    SyscallResult sys_allocate_region(usize size, MemoryProt prot);
    SyscallResult sys_chdir(const char *path);
    SyscallResult sys_close(uint32 fd);
    SyscallResult sys_connect(const char *path);
    SyscallResult sys_create_pipe(uint32 *fds);
    SyscallResult sys_create_process(const char *path, const char **argv, FdPair *copy_fds);
    SyscallResult sys_create_server_socket(const char *path, uint32 backlog_limit);
    SyscallResult sys_dup_fd(uint32 src, uint32 dst);
    SyscallResult sys_exit(usize code) const;
    SyscallResult sys_getcwd(char *path) const;
    SyscallResult sys_getpid() const;
    SyscallResult sys_ioctl(uint32 fd, IoctlRequest request, void *arg);
    SyscallResult sys_mkdir(const char *path) const;
    SyscallResult sys_mmap(uint32 fd) const;
    SyscallResult sys_mount(const char *target, const char *fs_type) const;
    SyscallResult sys_open(const char *path, OpenMode mode);
    SyscallResult sys_poll(PollFd *fds, usize count);
    SyscallResult sys_putchar(char ch) const;
    SyscallResult sys_read(uint32 fd, void *data, usize size);
    SyscallResult sys_read_directory(const char *path, uint8 *data);
    SyscallResult sys_seek(uint32 fd, usize offset, SeekMode mode);
    SyscallResult sys_size(uint32 fd);
    SyscallResult sys_wait_pid(usize pid);
    SyscallResult sys_write(uint32 fd, void *data, usize size);

    usize pid() const { return m_pid; }
    bool is_kernel() const { return m_is_kernel; }
    FileHandle &file_handle(uint32 fd) { return *m_fds[fd]; }
    usize thread_count() const { return m_thread_count.load(MemoryOrder::Relaxed); }
};
