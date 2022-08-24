#include <kernel/ipc/DoubleBuffer.hh>

#include <kernel/ScopedLock.hh> // IWYU pragma: keep
#include <ustd/Numeric.hh>
#include <ustd/Span.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>

namespace kernel {

DoubleBuffer::DoubleBuffer(size_t size) : m_size(size), m_read_buffer(&m_buffer1), m_write_buffer(&m_buffer2) {
    m_data = new uint8_t[size * 2];
    m_read_buffer->data = m_data + size;
    m_write_buffer->data = m_data;
}

DoubleBuffer::~DoubleBuffer() {
    delete[] m_data;
}

bool DoubleBuffer::empty() const {
    ScopedLock locker(m_lock);
    return m_read_position >= m_read_buffer->size && m_write_buffer->size == 0;
}

bool DoubleBuffer::full() const {
    ScopedLock locker(m_lock);
    return m_size - m_write_buffer->size == 0;
}

size_t DoubleBuffer::read(ustd::Span<void> data) {
    ScopedLock locker(m_lock);
    if (m_read_position >= m_read_buffer->size && m_write_buffer->size != 0) {
        ustd::swap(m_read_buffer, m_write_buffer);
        m_read_position = 0;
        m_write_buffer->size = 0;
    }
    if (m_read_position >= m_read_buffer->size) {
        return 0;
    }
    size_t read_size = ustd::min(data.size(), m_read_buffer->size - m_read_position);
    __builtin_memcpy(data.data(), m_read_buffer->data + m_read_position, read_size);
    m_read_position += read_size;
    return read_size;
}

size_t DoubleBuffer::write(ustd::Span<const void> data) {
    ScopedLock locker(m_lock);
    size_t write_size = ustd::min(data.size(), m_size - m_write_buffer->size);
    __builtin_memcpy(m_write_buffer->data + m_write_buffer->size, data.data(), write_size);
    m_write_buffer->size += write_size;
    return write_size;
}

} // namespace kernel
