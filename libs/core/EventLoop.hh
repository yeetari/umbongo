#pragma once

#include <kernel/SyscallTypes.hh>
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

namespace core {

class Timer;
class Watchable;

class EventLoop {
    Vector<PollFd> m_poll_fds;
    Vector<Timer *> m_timers;
    Vector<Watchable *> m_watchables;

    uint32 index_of(Timer *timer);
    uint32 index_of(Watchable *watchable);
    ssize next_timer_deadline();

public:
    void register_timer(Timer &timer);
    void unregister_timer(Timer &timer);
    void watch(Watchable &watchable, PollEvents events);
    void unwatch(Watchable &watchable);
    usize run();
};

} // namespace core
