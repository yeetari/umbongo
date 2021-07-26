#include <kernel/fs/Pipe.hh>

#include <ustd/Memory.hh>
#include <ustd/Numeric.hh>
#include <ustd/Span.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>

namespace {

constexpr usize k_buffer_size = 64_KiB;

} // namespace

Pipe::Pipe() : m_read_buffer(&m_buffer1), m_write_buffer(&m_buffer2) {
    m_data = new uint8[k_buffer_size * 2];
    m_read_buffer->data = m_data + k_buffer_size;
    m_write_buffer->data = m_data;
}

Pipe::~Pipe() {
    delete[] m_data;
}

usize Pipe::read(Span<void> data, usize) {
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

usize Pipe::size() {
    return k_buffer_size;
}

usize Pipe::write(Span<const void> data, usize) {
    usize write_size = ustd::min(data.size(), k_buffer_size - m_write_buffer->size);
    memcpy(m_write_buffer->data + m_write_buffer->size, data.data(), write_size);
    m_write_buffer->size += write_size;
    return write_size;
}
