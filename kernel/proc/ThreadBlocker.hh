#pragma once

#include <kernel/fs/File.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>
#include <ustd/Vector.hh>
// IWYU pragma: no_include "kernel/proc/Process.hh"

struct PollFd;
class Process; // IWYU pragma: keep
class SpinLock;

class ThreadBlocker {
public:
    ThreadBlocker() = default;
    ThreadBlocker(const ThreadBlocker &) = delete;
    ThreadBlocker(ThreadBlocker &&) = delete;
    virtual ~ThreadBlocker() = default;

    ThreadBlocker &operator=(const ThreadBlocker &) = delete;
    ThreadBlocker &operator=(ThreadBlocker &&) = delete;

    virtual bool should_unblock() = 0;
};

class PollBlocker : public ThreadBlocker {
    const LargeVector<PollFd> &m_fds;
    SpinLock &m_lock;
    Process &m_process;

public:
    PollBlocker(const LargeVector<PollFd> &fds, SpinLock &lock, Process &process)
        : m_fds(fds), m_lock(lock), m_process(process) {}

    bool should_unblock() override;
};

class ReadBlocker : public ThreadBlocker {
    SharedPtr<File> m_file;

public:
    explicit ReadBlocker(File &file) : m_file(&file) {}

    bool should_unblock() override;
};

class WaitBlocker final : public ThreadBlocker {
    SharedPtr<Process> m_process;

public:
    explicit WaitBlocker(usize pid);

    bool should_unblock() override;
};

class WriteBlocker : public ThreadBlocker {
    SharedPtr<File> m_file;

public:
    explicit WriteBlocker(File &file) : m_file(&file) {}

    bool should_unblock() override;
};
