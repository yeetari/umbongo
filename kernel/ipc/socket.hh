#pragma once

#include <kernel/fs/file.hh>
#include <kernel/sys_result.hh>
#include <ustd/shared_ptr.hh>
#include <ustd/span.hh> // IWYU pragma: keep
#include <ustd/types.hh>

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

    bool read_would_block(size_t offset) const override;
    bool write_would_block(size_t offset) const override;
    SysResult<size_t> read(ustd::Span<void> data, size_t offset) override;
    SysResult<size_t> write(ustd::Span<const void> data, size_t offset) override;

    bool connected() const;
    DoubleBuffer *read_buffer() const { return m_read_buffer.ptr(); }
    DoubleBuffer *write_buffer() const { return m_write_buffer.ptr(); }
};

} // namespace kernel
