#pragma once

#include <kernel/cpu/RegisterState.hh>
#include <ustd/Types.hh>

struct Scheduler;
class VirtSpace;

enum class ProcessState {
    Alive,
    Dead,
};

class Process {
    friend Scheduler;

private:
    const usize m_pid;
    const bool m_is_kernel;
    VirtSpace *const m_virt_space;
    ProcessState m_state{ProcessState::Alive};
    RegisterState m_register_state{};
    Process *m_next{nullptr};

    Process(usize pid, bool is_kernel, VirtSpace *virt_space);

public:
    static Process *create_kernel();
    static Process *create_user(void *binary, usize binary_size);

    Process(const Process &) = delete;
    Process(Process &&) = delete;
    ~Process();

    Process &operator=(const Process &) = delete;
    Process &operator=(Process &&) = delete;

    void kill();
    void set_entry_point(uintptr entry);

    uint64 sys_exit(uint64 code) const;
    uint64 sys_getpid() const;
    uint64 sys_putchar(char ch) const;

    usize pid() const { return m_pid; }
    bool is_kernel() const { return m_is_kernel; }
    RegisterState &register_state() { return m_register_state; }
};
