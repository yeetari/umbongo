#include <core/EventLoop.hh>

#include <core/Error.hh>
#include <core/Timer.hh>
#include <core/Watchable.hh>
#include <kernel/Syscall.hh>
#include <kernel/SyscallTypes.hh>
#include <ustd/Assert.hh>
#include <ustd/Function.hh>
#include <ustd/Log.hh>
#include <ustd/Result.hh>
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

namespace core {

uint32 EventLoop::index_of(Timer *timer) {
    for (uint32 i = 0; i < m_timers.size(); i++) {
        if (m_timers[i] == timer) {
            return i;
        }
    }
    ENSURE_NOT_REACHED();
}

uint32 EventLoop::index_of(Watchable *watchable) {
    for (uint32 i = 0; i < m_watchables.size(); i++) {
        if (m_watchables[i] == watchable) {
            return i;
        }
    }
    ENSURE_NOT_REACHED();
}

ssize EventLoop::next_timer_deadline() {
    ssize deadline = -1;
    for (auto *timer : m_timers) {
        const auto period = static_cast<ssize>(timer->period());
        if (deadline == -1 || period < deadline) {
            deadline = period;
        }
    }
    return deadline;
}

void EventLoop::register_timer(Timer &timer) {
    m_timers.push(&timer);
}

void EventLoop::unregister_timer(Timer &timer) {
    m_timers.remove(index_of(&timer));
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
        const auto timeout = next_timer_deadline();
        if (auto rc = Syscall::invoke(Syscall::poll, m_poll_fds.data(), m_poll_fds.size(), timeout); rc.is_error()) {
            dbgln("poll: {}", core::error_string(rc.error()));
            return 1;
        }
        auto now = EXPECT(Syscall::invoke<usize>(Syscall::gettime));
        for (auto *timer : m_timers) {
            if (!timer->has_expired(now)) {
                continue;
            }
            if (timer->m_on_fire) {
                timer->m_on_fire();
            }
            timer->reload(now);
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
