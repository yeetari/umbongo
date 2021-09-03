#pragma once

#include <kernel/SyscallTypes.hh>
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

namespace core {

class Watchable;

class EventLoop {
    Vector<PollFd> m_poll_fds;
    Vector<Watchable *> m_watchables;

    uint32 index_of(Watchable *watchable);

public:
    void watch(Watchable &watchable, PollEvents events);
    void unwatch(Watchable &watchable);
    usize run();
};

} // namespace core
