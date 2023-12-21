#pragma once

#include <ustd/utility.hh>

namespace ustd {

template <typename Callback>
class ScopeGuard {
    const Callback m_callback;
    bool m_armed{true};

public:
    ScopeGuard(Callback callback) : m_callback(move(callback)) {}
    ScopeGuard(const ScopeGuard &) = delete;
    ScopeGuard(ScopeGuard &&) = delete;
    ~ScopeGuard() {
        if (m_armed) {
            m_callback();
        }
    }

    ScopeGuard &operator=(const ScopeGuard &) = delete;
    ScopeGuard &operator=(ScopeGuard &&) = delete;

    void disarm() { m_armed = false; }
};

template <typename Callback>
ScopeGuard(Callback) -> ScopeGuard<Callback>;

} // namespace ustd
