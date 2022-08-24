#include <core/Timer.hh>

#include <core/EventLoop.hh>
#include <ustd/Types.hh>

namespace core {

Timer::Timer(EventLoop &event_loop, size_t period) : m_event_loop(event_loop), m_period(period) {
    event_loop.register_timer(*this);
}

Timer::~Timer() {
    m_event_loop.unregister_timer(*this);
}

} // namespace core
