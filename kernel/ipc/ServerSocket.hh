#pragma once

#include <kernel/SpinLock.hh>
#include <kernel/SysResult.hh>
#include <kernel/fs/File.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Span.hh> // IWYU pragma: keep
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

namespace kernel {

class Socket;

class ServerSocket final : public File {
    ustd::Vector<ustd::SharedPtr<Socket>> m_connection_queue;
    mutable SpinLock m_lock;

public:
    explicit ServerSocket(uint32 backlog_limit);
    ServerSocket(const ServerSocket &) = delete;
    ServerSocket(ServerSocket &&) = delete;
    ~ServerSocket() override;

    ServerSocket &operator=(const ServerSocket &) = delete;
    ServerSocket &operator=(ServerSocket &&) = delete;

    bool is_server_socket() const override { return true; }

    ustd::SharedPtr<Socket> accept();
    bool accept_would_block() const;
    bool read_would_block(usize) const override;
    bool write_would_block(usize) const override;
    SysResult<> queue_connection_from(ustd::SharedPtr<Socket> socket);
    SysResult<usize> read(ustd::Span<void> data, usize offset) override;
    SysResult<usize> write(ustd::Span<const void> data, usize offset) override;
};

} // namespace kernel
