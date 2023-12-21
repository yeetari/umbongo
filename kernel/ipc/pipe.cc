#include <kernel/ipc/pipe.hh>

#include <kernel/fs/file.hh>
#include <kernel/ipc/double_buffer.hh>
#include <kernel/scoped_lock.hh>
#include <kernel/spin_lock.hh>
#include <kernel/sys_result.hh>
#include <ustd/assert.hh>
#include <ustd/span.hh>
#include <ustd/types.hh>

namespace kernel {
namespace {

constexpr size_t k_buffer_size = 64_KiB;

} // namespace

Pipe::Pipe() : m_buffer(k_buffer_size) {}

void Pipe::attach(AttachDirection direction) {
    ASSERT(direction != AttachDirection::ReadWrite);
    ScopedLock locker(m_lock);
    if (direction == AttachDirection::Read) {
        m_reader_count++;
    } else if (direction == AttachDirection::Write) {
        m_writer_count++;
    }
}

void Pipe::detach(AttachDirection direction) {
    ASSERT(direction != AttachDirection::ReadWrite);
    ScopedLock locker(m_lock);
    if (direction == AttachDirection::Read) {
        ASSERT(m_reader_count > 0);
        m_reader_count--;
    } else if (direction == AttachDirection::Write) {
        ASSERT(m_writer_count > 0);
        m_writer_count--;
    }
}

bool Pipe::read_would_block(size_t) const {
    ScopedLock locker(m_lock);
    return m_writer_count != 0 && m_buffer.empty();
}

bool Pipe::write_would_block(size_t) const {
    return m_buffer.full();
}

SysResult<size_t> Pipe::read(ustd::Span<void> data, size_t) {
    return m_buffer.read(data);
}

SysResult<size_t> Pipe::write(ustd::Span<const void> data, size_t) {
    return m_buffer.write(data);
}

} // namespace kernel
