#include <kernel/ipc/Socket.hh>

#include <kernel/ScopedLock.hh> // IWYU pragma: keep
#include <kernel/SysResult.hh>
#include <kernel/ipc/DoubleBuffer.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Span.hh>
#include <ustd/Types.hh>

Socket::Socket(DoubleBuffer *read_buffer, DoubleBuffer *write_buffer)
    : m_read_buffer(read_buffer), m_write_buffer(write_buffer) {}

Socket::~Socket() = default;

bool Socket::can_read() {
    return m_read_buffer->ref_count() != 2 || !m_read_buffer->empty();
}

bool Socket::can_write() {
    return m_write_buffer->ref_count() != 2 || !m_write_buffer->full();
}

SysResult<usize> Socket::read(ustd::Span<void> data, usize) {
    return m_read_buffer->read(data);
}

SysResult<usize> Socket::write(ustd::Span<const void> data, usize) {
    return m_write_buffer->write(data);
}

bool Socket::connected() const {
    return m_read_buffer->ref_count() == 2;
}
