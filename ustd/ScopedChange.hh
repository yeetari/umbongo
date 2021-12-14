#pragma once

namespace ustd {

template <typename T>
class ScopedChange {
    T &m_value;
    T m_old_value;

public:
    template <typename U>
    ScopedChange(T &value, U new_value) : m_value(value), m_old_value(value) {
        m_value = new_value;
    }
    ScopedChange(const ScopedChange &) = delete;
    ScopedChange(ScopedChange &&) = delete;
    ~ScopedChange() { m_value = m_old_value; }

    ScopedChange &operator=(const ScopedChange &) = delete;
    ScopedChange &operator=(ScopedChange &&) = delete;
};

template <typename T>
ScopedChange(T) -> ScopedChange<T>;

} // namespace ustd
