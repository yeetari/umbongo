#pragma once

#include <ustd/Utility.hh>

namespace ustd {

template <typename Callback>
class ScopeGuard {
    const Callback m_callback;

public:
    ScopeGuard(Callback callback) : m_callback(move(callback)) {}
    ScopeGuard(const ScopeGuard &) = delete;
    ScopeGuard(ScopeGuard &&) = delete;
    ~ScopeGuard() { m_callback(); }

    ScopeGuard &operator=(const ScopeGuard &) = delete;
    ScopeGuard &operator=(ScopeGuard &&) = delete;
};

template <typename Callback>
ScopeGuard(Callback) -> ScopeGuard<Callback>;

} // namespace ustd
