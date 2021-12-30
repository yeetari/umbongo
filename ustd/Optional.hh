#pragma once

#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/Memory.hh>
#include <ustd/Traits.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>

namespace ustd {

template <typename T>
class Optional {
    alignas(T) Array<uint8, sizeof(T)> m_data{};
    bool m_present{false};

public:
    constexpr Optional() = default;
    constexpr Optional(const T &value) : m_present(true) { new (m_data.data()) T(value); }  // NOLINT
    constexpr Optional(T &&value) : m_present(true) { new (m_data.data()) T(move(value)); } // NOLINT
    constexpr Optional(const Optional &) = delete;
    constexpr Optional(Optional &&other) noexcept requires(IsMoveConstructible<T>) : m_present(other.m_present) {
        if (other) {
            new (m_data.data()) T(move(*other));
            other.clear();
        }
    }
    constexpr ~Optional() { clear(); }

    Optional &operator=(const Optional &) = delete;
    Optional &operator=(Optional &&other) noexcept {
        if (this != &other) {
            clear();
            m_present = other.m_present;
            if (other) {
                new (m_data.data()) T(move(*other));
                other.clear();
            }
        }
        return *this;
    }

    constexpr void clear();
    constexpr T disown_value();
    template <typename... Args>
    constexpr T &emplace(Args &&...args);
    template <typename U>
    constexpr T value_or(U &&fallback) const &;
    template <typename U>
    constexpr T value_or(U &&fallback) &&;

    constexpr explicit operator bool() const noexcept { return m_present; }
    constexpr bool has_value() const noexcept { return m_present; }

    constexpr T &operator*() {
        ASSERT(m_present);
        return *reinterpret_cast<T *>(m_data.data());
    }
    constexpr T *operator->() {
        ASSERT(m_present);
        return reinterpret_cast<T *>(m_data.data());
    }
    constexpr const T &operator*() const {
        ASSERT(m_present);
        return *reinterpret_cast<const T *>(m_data.data());
    }
    constexpr const T *operator->() const {
        ASSERT(m_present);
        return reinterpret_cast<const T *>(m_data.data());
    }
    constexpr T *obj() { return reinterpret_cast<T *>(m_data.data()); }
};

template <typename T>
constexpr void Optional<T>::clear() {
    if constexpr (!IsTriviallyDestructible<T>) {
        if (m_present) {
            operator*().~T();
        }
    }
    m_present = false;
}

template <typename T>
constexpr T Optional<T>::disown_value() {
    auto disowned_value = move(operator*());
    clear();
    return disowned_value;
}

template <typename T>
template <typename... Args>
constexpr T &Optional<T>::emplace(Args &&...args) {
    clear();
    new (m_data.data()) T(forward<Args>(args)...);
    m_present = true;
    return operator*();
}

template <typename T>
template <typename U>
constexpr T Optional<T>::value_or(U &&fallback) const & {
    return m_present ? **this : static_cast<T>(forward<U>(fallback));
}

template <typename T>
template <typename U>
constexpr T Optional<T>::value_or(U &&fallback) && {
    return m_present ? move(disown_value()) : static_cast<T>(forward<U>(fallback));
}

} // namespace ustd
