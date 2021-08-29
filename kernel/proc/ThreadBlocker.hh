#pragma once

#include <kernel/fs/File.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>
// IWYU pragma: no_include "kernel/proc/Process.hh"

class Process; // IWYU pragma: keep

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

class ReadBlocker : public ThreadBlocker {
    SharedPtr<File> m_file;

public:
    explicit ReadBlocker(SharedPtr<File> file) : m_file(ustd::move(file)) {}

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
    explicit WriteBlocker(SharedPtr<File> file) : m_file(ustd::move(file)) {}

    bool should_unblock() override;
};
