#pragma once

#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/Concepts.hh>
#include <ustd/Traits.hh>

namespace ustd {

template <typename T, typename E>
class Result {
    union {
        E m_error;
        alignas(T) Array<uint8, sizeof(T)> m_value;
    };
    const bool m_is_error;

public:
    Result(E error) : m_error(error), m_is_error(true) {}
    template <ConvertibleTo<T> U>
    Result(U &&value) : m_is_error(false) {
        new (m_value.data()) T(forward<U>(value));
    }
    Result(const Result &) = delete;
    Result(Result &&) = delete;
    ~Result();

    Result &operator=(const Result &) = delete;
    Result &operator=(Result &&) = delete;

    bool is_error() const { return m_is_error; }
    E error() const {
        ASSERT(m_is_error);
        return m_error;
    }
    T &operator*() {
        ASSERT(!m_is_error);
        return *reinterpret_cast<T *>(m_value.data());
    }
    T operator->() requires(IsPointer<T>) {
        ASSERT(!m_is_error);
        return *reinterpret_cast<T *>(m_value.data());
    }
    T *operator->() requires(!IsPointer<T>) {
        ASSERT(!m_is_error);
        return reinterpret_cast<T *>(m_value.data());
    }
};

template <typename T, typename E>
Result<T, E>::~Result() {
    if constexpr (!IsTriviallyDestructible<T>) {
        if (!m_is_error) {
            operator*().~T();
        }
    }
}

} // namespace ustd

using ustd::Result;
