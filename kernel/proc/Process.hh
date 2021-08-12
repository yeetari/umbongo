#pragma once

#include <kernel/SysResult.hh>
#include <kernel/SyscallTypes.hh>
#include <kernel/cpu/RegisterState.hh>
#include <kernel/fs/FileHandle.hh>
#include <kernel/mem/VirtSpace.hh> // IWYU pragma: keep
#include <ustd/Optional.hh>        // IWYU pragma: keep
#include <ustd/SharedPtr.hh>
#include <ustd/String.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

struct Scheduler;

enum class ProcessState {
    Alive,
    Dead,
};

class Process {
    friend Scheduler;

private:
    const usize m_pid;
    const bool m_is_kernel;
    SharedPtr<VirtSpace> m_virt_space;

    Process *m_prev{nullptr};
    Process *m_next{nullptr};
    ProcessState m_state{ProcessState::Alive};
    RegisterState m_register_state{};
    Vector<Optional<FileHandle>> m_fds;

    Process(usize pid, bool is_kernel, SharedPtr<VirtSpace> virt_space);

    uint32 allocate_fd();

public:
    static Process *create_kernel();
    static Process *create_user();

    Process(const Process &) = delete;
    Process(Process &&) = delete;
    ~Process();

    Process &operator=(const Process &) = delete;
    Process &operator=(Process &&) = delete;

    SysResult exec(StringView path, const Vector<String> &args = {});
    void kill();
    void set_entry_point(uintptr entry);

    SysResult sys_allocate_region(usize size, MemoryProt prot);
    SysResult sys_close(uint32 fd);
    SysResult sys_create_pipe(uint32 *fds);
    SysResult sys_create_process(const char *path, const char **argv, FdPair *copy_fds);
    SysResult sys_dup_fd(uint32 src, uint32 dst);
    SysResult sys_exit(usize code) const;
    SysResult sys_getpid() const;
    SysResult sys_ioctl(uint32 fd, IoctlRequest request, void *arg);
    SysResult sys_is_alive(usize pid);
    SysResult sys_mkdir(const char *path) const;
    SysResult sys_mmap(uint32 fd) const;
    SysResult sys_mount(const char *target, const char *fs_type) const;
    SysResult sys_open(const char *path, OpenMode mode);
    SysResult sys_putchar(char ch) const;
    SysResult sys_read(uint32 fd, void *data, usize size);
    SysResult sys_seek(uint32 fd, usize offset, SeekMode mode);
    SysResult sys_size(uint32 fd);
    SysResult sys_write(uint32 fd, void *data, usize size);

    usize pid() const { return m_pid; }
    bool is_kernel() const { return m_is_kernel; }
    RegisterState &register_state() { return m_register_state; }
};
