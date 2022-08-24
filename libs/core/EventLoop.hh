#pragma once

#include <kernel/SyscallTypes.hh>
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

namespace core {

class Timer;
class Watchable;

class EventLoop {
    ustd::Vector<kernel::PollFd> m_poll_fds;
    ustd::Vector<Timer *> m_timers;
    ustd::Vector<Watchable *> m_watchables;

    uint32_t index_of(Timer *timer);
    uint32_t index_of(Watchable *watchable);
    ssize_t next_timer_deadline();

public:
    void register_timer(Timer &timer);
    void unregister_timer(Timer &timer);
    void watch(Watchable &watchable, kernel::PollEvents events);
    void unwatch(Watchable &watchable);
    size_t run();
};

} // namespace core
