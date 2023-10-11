#include <kernel/ipc/Socket.hh>

#include <kernel/SysResult.hh>
#include <kernel/ipc/DoubleBuffer.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Span.hh>
#include <ustd/Types.hh>

namespace kernel {

Socket::Socket(DoubleBuffer *read_buffer, DoubleBuffer *write_buffer)
    : m_read_buffer(read_buffer), m_write_buffer(write_buffer) {}

Socket::~Socket() = default;

bool Socket::read_would_block(size_t) const {
    return m_read_buffer->ref_count() == 2 && m_read_buffer->empty();
}

bool Socket::write_would_block(size_t) const {
    return m_write_buffer->ref_count() == 2 && m_write_buffer->full();
}

SysResult<size_t> Socket::read(ustd::Span<void> data, size_t) {
    return m_read_buffer->read(data);
}

SysResult<size_t> Socket::write(ustd::Span<const void> data, size_t) {
    return m_write_buffer->write(data);
}

bool Socket::connected() const {
    return m_read_buffer->ref_count() == 2;
}

} // namespace kernel
