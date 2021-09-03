#pragma once

#include <kernel/SpinLock.hh>
#include <kernel/SysResult.hh>
#include <kernel/fs/File.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Span.hh> // IWYU pragma: keep
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

class Socket;

class ServerSocket final : public File {
    Vector<SharedPtr<Socket>> m_connection_queue;
    mutable SpinLock m_lock;

public:
    explicit ServerSocket(uint32 backlog_limit);
    ServerSocket(const ServerSocket &) = delete;
    ServerSocket(ServerSocket &&) = delete;
    ~ServerSocket() override;

    ServerSocket &operator=(const ServerSocket &) = delete;
    ServerSocket &operator=(ServerSocket &&) = delete;

    bool is_server_socket() const override { return true; }

    SharedPtr<Socket> accept();
    bool can_accept() const;
    bool can_read() override;
    bool can_write() override;
    SysResult<> queue_connection_from(SharedPtr<Socket> socket);
    SysResult<usize> read(Span<void> data, usize offset) override;
    SysResult<usize> write(Span<const void> data, usize offset) override;
};
