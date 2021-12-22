#pragma once

#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/Concepts.hh>
#include <ustd/Memory.hh>
#include <ustd/Traits.hh>
#include <ustd/Utility.hh>

#define EXPECT(expr, ...)                                                                                              \
    ({                                                                                                                 \
        auto _result_expect = (expr);                                                                                  \
        ENSURE(!_result_expect.is_error(), ##__VA_ARGS__);                                                             \
        _result_expect.value();                                                                                        \
    })

#define TRY(expr)                                                                                                      \
    ({                                                                                                                 \
        auto _result_try = (expr);                                                                                     \
        if (_result_try.is_error()) {                                                                                  \
            return _result_try.error();                                                                                \
        }                                                                                                              \
        _result_try.value();                                                                                           \
    })

namespace ustd {

template <typename T, typename E>
class [[nodiscard]] Result {
    union {
        E m_error;
        alignas(T) Array<uint8, sizeof(T)> m_value;
    };
    bool m_is_error;

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

    bool operator==(E error) const { return m_is_error && m_error == error; }
    bool operator==(const T &value) const {
        return !m_is_error && *reinterpret_cast<const T *>(m_value.data()) == value;
    }

    bool is_error() const { return m_is_error; }
    E error() const {
        ASSERT(m_is_error);
        return m_error;
    }
    T value() {
        ASSERT(!m_is_error);
        m_is_error = true;
        return move(*reinterpret_cast<T *>(m_value.data()));
    }
};

template <typename E>
class [[nodiscard]] Result<void, E> {
    E m_error{};
    const bool m_is_error{false};

public:
    Result() = default;
    Result(E error) : m_error(error), m_is_error(true) {}
    Result(const Result &) = delete;
    Result(Result &&) = delete;
    ~Result() = default;

    Result &operator=(const Result &) = delete;
    Result &operator=(Result &&) = delete;

    bool is_error() const { return m_is_error; }
    E error() const {
        ASSERT(m_is_error);
        return m_error;
    }
    void value() const {}
};

template <typename T, typename E>
Result<T, E>::~Result() {
    if constexpr (!IsTriviallyDestructible<T>) {
        if (!m_is_error) {
            reinterpret_cast<T *>(m_value.data())->~T();
        }
    }
}

} // namespace ustd
