#include <core/timer.hh>

#include <core/event_loop.hh>
#include <ustd/types.hh>

namespace core {

Timer::Timer(EventLoop &event_loop, size_t period) : m_event_loop(event_loop), m_period(period) {
    event_loop.register_timer(*this);
}

Timer::~Timer() {
    m_event_loop.unregister_timer(*this);
}

} // namespace core
