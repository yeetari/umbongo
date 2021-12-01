#include <kernel/ipc/DoubleBuffer.hh>

#include <kernel/ScopedLock.hh> // IWYU pragma: keep
#include <ustd/Memory.hh>
#include <ustd/Numeric.hh>
#include <ustd/Span.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>

DoubleBuffer::DoubleBuffer(usize size) : m_size(size), m_read_buffer(&m_buffer1), m_write_buffer(&m_buffer2) {
    m_data = new uint8[size * 2];
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

usize DoubleBuffer::read(ustd::Span<void> data) {
    ScopedLock locker(m_lock);
    if (m_read_position >= m_read_buffer->size && m_write_buffer->size != 0) {
        ustd::swap(m_read_buffer, m_write_buffer);
        m_read_position = 0;
        m_write_buffer->size = 0;
    }
    if (m_read_position >= m_read_buffer->size) {
        return 0;
    }
    usize read_size = ustd::min(data.size(), m_read_buffer->size - m_read_position);
    memcpy(data.data(), m_read_buffer->data + m_read_position, read_size);
    m_read_position += read_size;
    return read_size;
}

usize DoubleBuffer::write(ustd::Span<const void> data) {
    ScopedLock locker(m_lock);
    usize write_size = ustd::min(data.size(), m_size - m_write_buffer->size);
    memcpy(m_write_buffer->data + m_write_buffer->size, data.data(), write_size);
    m_write_buffer->size += write_size;
    return write_size;
}
