#pragma once

#include <kernel/cpu/RegisterState.hh>
#include <kernel/fs/FileHandle.hh>
#include <kernel/mem/VirtSpace.hh>
#include <ustd/Optional.hh> // IWYU pragma: keep
#include <ustd/SharedPtr.hh>
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
    ~Process() = default;

    Process &operator=(const Process &) = delete;
    Process &operator=(Process &&) = delete;

    void exec(StringView path);
    void kill();
    void set_entry_point(uintptr entry);

    uint64 sys_close(uint32 fd);
    uint64 sys_exit(uint64 code) const;
    uint64 sys_getpid() const;
    uint64 sys_open(const char *path);
    uint64 sys_putchar(char ch) const;
    uint64 sys_read(uint32 fd, void *data, usize size) const;
    uint64 sys_write(uint32 fd, void *data, usize size) const;

    usize pid() const { return m_pid; }
    bool is_kernel() const { return m_is_kernel; }
    RegisterState &register_state() { return m_register_state; }
};
