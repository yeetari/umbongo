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

    SysResult sys_allocate_region(usize size, MemoryProt prot);
    SysResult sys_close(uint32 fd);
    SysResult sys_create_pipe(uint32 *fds);
    SysResult sys_create_process(const char *path, const char **argv, FdPair *copy_fds);
    SysResult sys_dup_fd(uint32 src, uint32 dst);
    SysResult sys_exit(usize code) const;
    SysResult sys_getpid() const;
    SysResult sys_ioctl(uint32 fd, IoctlRequest request, void *arg);
    SysResult sys_mkdir(const char *path) const;
    SysResult sys_mmap(uint32 fd) const;
    SysResult sys_mount(const char *target, const char *fs_type) const;
    SysResult sys_open(const char *path, OpenMode mode);
    SysResult sys_putchar(char ch) const;
    SysResult sys_read(uint32 fd, void *data, usize size);
    SysResult sys_seek(uint32 fd, usize offset, SeekMode mode);
    SysResult sys_size(uint32 fd);
    SysResult sys_wait_pid(usize pid);
    SysResult sys_write(uint32 fd, void *data, usize size);

    usize pid() const { return m_pid; }
    bool is_kernel() const { return m_is_kernel; }
    usize thread_count() const { return m_thread_count.load(MemoryOrder::Relaxed); }
};
