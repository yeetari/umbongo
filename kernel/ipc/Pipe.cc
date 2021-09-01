#include <kernel/ipc/Pipe.hh>

#include <kernel/ScopedLock.hh> // IWYU pragma: keep
#include <kernel/fs/File.hh>
#include <kernel/ipc/DoubleBuffer.hh>
#include <ustd/Assert.hh>
#include <ustd/Span.hh>
#include <ustd/Types.hh>

namespace {

constexpr usize k_buffer_size = 64_KiB;

} // namespace

Pipe::Pipe() : m_buffer(k_buffer_size) {}

void Pipe::attach(AttachDirection direction) {
    ASSERT(direction != AttachDirection::ReadWrite);
    if (direction == AttachDirection::Read) {
        m_reader_count++;
    } else if (direction == AttachDirection::Write) {
        m_writer_count++;
    }
}

void Pipe::detach(AttachDirection direction) {
    ASSERT(direction != AttachDirection::ReadWrite);
    if (direction == AttachDirection::Read) {
        ASSERT(m_reader_count > 0);
        m_reader_count--;
    } else if (direction == AttachDirection::Write) {
        ASSERT(m_writer_count > 0);
        m_writer_count--;
    }
}

bool Pipe::can_read() {
    ScopedLock locker(m_lock);
    return !m_buffer.empty();
}

bool Pipe::can_write() {
    ScopedLock locker(m_lock);
    return !m_buffer.full();
}

usize Pipe::read(Span<void> data, usize) {
    ScopedLock locker(m_lock);
    return m_buffer.read(data);
}

usize Pipe::size() {
    return k_buffer_size;
}

usize Pipe::write(Span<const void> data, usize) {
    ScopedLock locker(m_lock);
    return m_buffer.write(data);
}
