#include <kernel/ipc/server_socket.hh>

#include <kernel/error.hh>
#include <kernel/ipc/socket.hh>
#include <kernel/scoped_lock.hh>
#include <kernel/spin_lock.hh>
#include <kernel/sys_result.hh>
#include <ustd/shared_ptr.hh>
#include <ustd/span.hh>
#include <ustd/types.hh>
#include <ustd/utility.hh>
#include <ustd/vector.hh>

namespace kernel {

ServerSocket::ServerSocket(uint32_t backlog_limit) {
    m_connection_queue.ensure_capacity(backlog_limit);
}

ServerSocket::~ServerSocket() = default;

ustd::SharedPtr<Socket> ServerSocket::accept() {
    ScopedLock locker(m_lock);
    auto client = m_connection_queue.take(0);
    return ustd::make_shared<Socket>(client->write_buffer(), client->read_buffer());
}

bool ServerSocket::accept_would_block() const {
    ScopedLock locker(m_lock);
    return m_connection_queue.empty();
}

bool ServerSocket::read_would_block(size_t) const {
    return accept_would_block();
}

bool ServerSocket::write_would_block(size_t) const {
    return false;
}

SysResult<> ServerSocket::queue_connection_from(ustd::SharedPtr<Socket> socket) {
    ScopedLock locker(m_lock);
    if (m_connection_queue.size() + 1 >= m_connection_queue.capacity()) {
        return Error::Busy;
    }
    m_connection_queue.push(ustd::move(socket));
    return {};
}

SysResult<size_t> ServerSocket::read(ustd::Span<void>, size_t) {
    return Error::Invalid;
}

SysResult<size_t> ServerSocket::write(ustd::Span<const void>, size_t) {
    return Error::Invalid;
}

} // namespace kernel
