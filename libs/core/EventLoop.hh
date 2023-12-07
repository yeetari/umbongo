#pragma once

#include <kernel/api/Types.h>
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

namespace core {

class Timer;
class Watchable;

class EventLoop {
    ustd::Vector<ub_poll_fd_t> m_poll_fds;
    ustd::Vector<Timer *> m_timers;
    ustd::Vector<Watchable *> m_watchables;

    uint32_t index_of(Timer *timer);
    uint32_t index_of(Watchable *watchable);
    ssize_t next_timer_deadline();

public:
    void register_timer(Timer &timer);
    void unregister_timer(Timer &timer);
    void watch(Watchable &watchable, ub_poll_events_t events);
    void unwatch(Watchable &watchable);
    size_t run();
};

} // namespace core
