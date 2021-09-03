#pragma once

#include <ustd/Function.hh>
#include <ustd/Types.hh>

namespace core {

class EventLoop;

class Watchable {
    friend EventLoop;

private:
    Function<void()> m_on_read_ready;
    Function<void()> m_on_write_ready;

public:
    Watchable() = default;
    Watchable(const Watchable &) = delete;
    Watchable(Watchable &&) = delete;
    virtual ~Watchable() = default;

    Watchable &operator=(const Watchable &) = delete;
    Watchable &operator=(Watchable &&) = delete;

    void set_on_read_ready(Function<void()> on_read_ready) { m_on_read_ready = ustd::move(on_read_ready); }
    void set_on_write_ready(Function<void()> on_write_ready) { m_on_write_ready = ustd::move(on_write_ready); }

    virtual uint32 fd() const = 0;
};

} // namespace core
