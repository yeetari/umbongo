#include <core/EventLoop.hh>

#include <core/Error.hh>
#include <core/Watchable.hh>
#include <kernel/Syscall.hh>
#include <kernel/SyscallTypes.hh>
#include <ustd/Assert.hh>
#include <ustd/Function.hh>
#include <ustd/Log.hh>
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

namespace core {

uint32 EventLoop::index_of(Watchable *watchable) {
    for (uint32 i = 0; i < m_watchables.size(); i++) {
        if (m_watchables[i] == watchable) {
            return i;
        }
    }
    ENSURE_NOT_REACHED();
}

void EventLoop::watch(Watchable &watchable, PollEvents events) {
    m_poll_fds.push(PollFd{
        .fd = watchable.fd(),
        .events = events,
    });
    m_watchables.push(&watchable);
}

void EventLoop::unwatch(Watchable &watchable) {
    uint32 index = index_of(&watchable);
    m_poll_fds.remove(index);
    m_watchables.remove(index);
}

usize EventLoop::run() {
    while (true) {
        if (auto rc = Syscall::invoke(Syscall::poll, m_poll_fds.data(), m_poll_fds.size()); rc < 0) {
            dbgln("poll: {}", core::error_string(rc));
            return 1;
        }
        for (uint32 i = m_poll_fds.size(); i > 0; i--) {
            auto &poll_fd = m_poll_fds[i - 1];
            auto *watchable = m_watchables[i - 1];
            ASSERT(poll_fd.fd == watchable->fd());
            if ((poll_fd.revents & PollEvents::Read) == PollEvents::Read && watchable->m_on_read_ready) {
                watchable->m_on_read_ready();
            }
            if ((poll_fd.revents & PollEvents::Write) == PollEvents::Write && watchable->m_on_write_ready) {
                watchable->m_on_write_ready();
            }
        }
    }
}

} // namespace core
