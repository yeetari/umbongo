#pragma once

#include <kernel/SysResult.hh>
#include <kernel/fs/File.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Span.hh> // IWYU pragma: keep
#include <ustd/Types.hh>

namespace kernel {

class DoubleBuffer;

class Socket final : public File {
    ustd::SharedPtr<DoubleBuffer> m_read_buffer;
    ustd::SharedPtr<DoubleBuffer> m_write_buffer;

public:
    Socket(DoubleBuffer *read_buffer, DoubleBuffer *write_buffer);
    Socket(const Socket &) = delete;
    Socket(Socket &&) = delete;
    ~Socket() override;

    Socket &operator=(const Socket &) = delete;
    Socket &operator=(Socket &&) = delete;

    bool is_socket() const override { return true; }

    bool can_read() override;
    bool can_write() override;
    SysResult<usize> read(ustd::Span<void> data, usize offset) override;
    SysResult<usize> write(ustd::Span<const void> data, usize offset) override;

    bool connected() const;
    DoubleBuffer *read_buffer() const { return m_read_buffer.obj(); }
    DoubleBuffer *write_buffer() const { return m_write_buffer.obj(); }
};

} // namespace kernel
