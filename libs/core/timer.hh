#pragma once

#include <ustd/function.hh>
#include <ustd/types.hh>
#include <ustd/utility.hh>

namespace core {

class EventLoop;

class Timer {
    friend EventLoop;

private:
    EventLoop &m_event_loop;
    ustd::Function<void()> m_on_fire;
    size_t m_fire_time{};
    size_t m_period;

public:
    Timer(EventLoop &event_loop, size_t period);
    Timer(const Timer &) = delete;
    Timer(Timer &&) = delete;
    ~Timer();

    Timer &operator=(const Timer &) = delete;
    Timer &operator=(Timer &&) = delete;

    bool has_expired(size_t now) const { return now > m_fire_time; }
    void reload(size_t now) { m_fire_time = now + m_period; }
    void set_on_fire(ustd::Function<void()> on_fire) { m_on_fire = ustd::move(on_fire); }
    void set_period(size_t period) { m_period = period; }

    size_t period() const { return m_period; }
};

} // namespace core

// NOLINTNEXTLINE
constexpr size_t operator"" _Hz(unsigned long long frequency) {
    return 1000000000ul / frequency;
}
