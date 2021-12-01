#pragma once

#include <ustd/Function.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>

namespace core {

class EventLoop;

class Timer {
    friend EventLoop;

private:
    EventLoop &m_event_loop;
    ustd::Function<void()> m_on_fire;
    usize m_fire_time{};
    usize m_period;

public:
    Timer(EventLoop &event_loop, usize period);
    Timer(const Timer &) = delete;
    Timer(Timer &&) = delete;
    ~Timer();

    Timer &operator=(const Timer &) = delete;
    Timer &operator=(Timer &&) = delete;

    bool has_expired(usize now) const { return now > m_fire_time; }
    void reload(usize now) { m_fire_time = now + m_period; }
    void set_on_fire(ustd::Function<void()> on_fire) { m_on_fire = ustd::move(on_fire); }

    usize period() const { return m_period; }
};

} // namespace core

// NOLINTNEXTLINE
constexpr usize operator"" _Hz(unsigned long long frequency) {
    return 1000000000ul / frequency;
}
