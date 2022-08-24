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
    explicit ServerSocket(uint32_t backlog_limit);
    ServerSocket(const ServerSocket &) = delete;
    ServerSocket(ServerSocket &&) = delete;
    ~ServerSocket() override;

    ServerSocket &operator=(const ServerSocket &) = delete;
    ServerSocket &operator=(ServerSocket &&) = delete;

    bool is_server_socket() const override { return true; }

    ustd::SharedPtr<Socket> accept();
    bool accept_would_block() const;
    bool read_would_block(size_t) const override;
    bool write_would_block(size_t) const override;
    SysResult<> queue_connection_from(ustd::SharedPtr<Socket> socket);
    SysResult<size_t> read(ustd::Span<void> data, size_t offset) override;
    SysResult<size_t> write(ustd::Span<const void> data, size_t offset) override;
};

} // namespace kernel
