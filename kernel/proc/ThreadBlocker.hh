#pragma once

#include <kernel/fs/File.hh>
#include <kernel/ipc/ServerSocket.hh>
#include <kernel/ipc/Socket.hh>
#include <kernel/proc/Process.hh>
#include <ustd/Optional.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>
#include <ustd/Vector.hh>

namespace kernel {

struct PollFd;
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

class AcceptBlocker : public ThreadBlocker {
    ustd::SharedPtr<ServerSocket> m_server;

public:
    explicit AcceptBlocker(ustd::SharedPtr<ServerSocket> server) : m_server(ustd::move(server)) {}

    bool should_unblock() override;
};

class ConnectBlocker : public ThreadBlocker {
    ustd::SharedPtr<Socket> m_socket;

public:
    explicit ConnectBlocker(ustd::SharedPtr<Socket> socket) : m_socket(ustd::move(socket)) {}

    bool should_unblock() override;
};

class PollBlocker : public ThreadBlocker {
    const ustd::LargeVector<PollFd> &m_fds;
    SpinLock &m_lock;
    Process &m_process;
    ustd::Optional<size_t> m_deadline;

public:
    PollBlocker(const ustd::LargeVector<PollFd> &fds, SpinLock &lock, Process &process, ssize_t timeout);

    bool should_unblock() override;
};

class ReadBlocker : public ThreadBlocker {
    ustd::SharedPtr<File> m_file;
    size_t m_offset;

public:
    ReadBlocker(File &file, size_t offset) : m_file(&file), m_offset(offset) {}

    bool should_unblock() override;
};

class WaitBlocker final : public ThreadBlocker {
    ustd::SharedPtr<Process> m_process;

public:
    explicit WaitBlocker(size_t pid);

    bool should_unblock() override;
};

class WriteBlocker : public ThreadBlocker {
    ustd::SharedPtr<File> m_file;
    size_t m_offset;

public:
    WriteBlocker(File &file, size_t offset) : m_file(&file), m_offset(offset) {}

    bool should_unblock() override;
};

} // namespace kernel
