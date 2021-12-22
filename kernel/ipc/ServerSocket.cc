#include <kernel/ipc/ServerSocket.hh>

#include <kernel/ScopedLock.hh> // IWYU pragma: keep
#include <kernel/SysError.hh>
#include <kernel/SysResult.hh>
#include <kernel/ipc/Socket.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Span.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>
#include <ustd/Vector.hh>

namespace kernel {

ServerSocket::ServerSocket(uint32 backlog_limit) {
    m_connection_queue.ensure_capacity(backlog_limit);
}

ServerSocket::~ServerSocket() = default;

ustd::SharedPtr<Socket> ServerSocket::accept() {
    ScopedLock locker(m_lock);
    auto client = m_connection_queue.take(0);
    return ustd::make_shared<Socket>(client->write_buffer(), client->read_buffer());
}

bool ServerSocket::can_accept() const {
    ScopedLock locker(m_lock);
    return !m_connection_queue.empty();
}

bool ServerSocket::can_read() {
    return can_accept();
}

bool ServerSocket::can_write() {
    return true;
}

SysResult<> ServerSocket::queue_connection_from(ustd::SharedPtr<Socket> socket) {
    ScopedLock locker(m_lock);
    if (m_connection_queue.size() + 1 >= m_connection_queue.capacity()) {
        return SysError::Busy;
    }
    m_connection_queue.push(ustd::move(socket));
    return {};
}

SysResult<usize> ServerSocket::read(ustd::Span<void>, usize) {
    return SysError::Invalid;
}

SysResult<usize> ServerSocket::write(ustd::Span<const void>, usize) {
    return SysError::Invalid;
}

} // namespace kernel
