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
    constexpr Optional(Optional &&) requires(IsMoveConstructible<T>);
    constexpr ~Optional() { clear(); }

    constexpr Optional &operator=(const Optional &) = delete;
    constexpr Optional &operator=(Optional &&);

    constexpr void clear();
    constexpr T disown_value();
    template <typename... Args>
    constexpr T &emplace(Args &&...args);
    template <typename U>
    constexpr T value_or(U &&fallback) const &;
    template <typename U>
    constexpr T value_or(U &&fallback) &&;

    constexpr explicit operator bool() const { return m_present; }
    constexpr bool has_value() const { return m_present; }

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
class Optional<T &> {
    class RefWrapper {
        T *m_ptr;

    public:
        RefWrapper(T &ptr) : m_ptr(&ptr) {}
        operator T &() { return *m_ptr; }
    };

    T *m_obj{nullptr};

public:
    constexpr Optional() = default;
    constexpr Optional(T &obj) : m_obj(&obj) {}
    constexpr Optional(const Optional &) = delete;
    constexpr Optional(Optional &&) = delete;
    constexpr ~Optional() = default;

    constexpr Optional &operator=(const Optional &) = delete;
    constexpr Optional &operator=(Optional &&) = delete;

    constexpr RefWrapper disown_value() { return operator*(); }
    constexpr explicit operator bool() const { return m_obj != nullptr; }
    constexpr bool has_value() const { return m_obj != nullptr; }

    constexpr T &operator*() {
        ASSERT(m_obj != nullptr);
        return *m_obj;
    }
    constexpr T *operator->() {
        ASSERT(m_obj != nullptr);
        return m_obj;
    }
    constexpr const T &operator*() const {
        ASSERT(m_obj != nullptr);
        return *m_obj;
    }
    constexpr const T *operator->() const {
        ASSERT(m_obj != nullptr);
        return m_obj;
    }
    constexpr T *obj() { return m_obj; }
};

template <typename T>
constexpr Optional<T>::Optional(Optional &&other) requires(IsMoveConstructible<T>) : m_present(other.m_present) {
    if (other) {
        new (m_data.data()) T(move(*other));
        other.clear();
    }
}

template <typename T>
constexpr Optional<T> &Optional<T>::operator=(Optional &&other) {
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
