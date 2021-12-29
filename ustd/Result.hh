#pragma once

#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/Concepts.hh>
#include <ustd/Memory.hh>
#include <ustd/Optional.hh>
#include <ustd/Traits.hh>
#include <ustd/Utility.hh>

namespace ustd {

class ErrorUnionBase {
private:
    usize m_type;
    usize m_error;

public:
    ErrorUnionBase() = default;
    ErrorUnionBase(usize type, usize error) : m_type(type), m_error(error) {}

    usize error() const { return m_error; }
    usize type() const { return m_type; }
};

template <unsigned N, typename... Es>
struct ErrorUnionImpl;

template <unsigned N>
struct ErrorUnionImpl<N> : ErrorUnionBase {
    using ErrorUnionBase::ErrorUnionBase;
};

template <unsigned N, typename E, typename... Es>
struct ErrorUnionImpl<N, E, Es...> : ErrorUnionImpl<N + 1, Es...> {
    using ErrorUnionImpl<N + 1, Es...>::ErrorUnionImpl;

    ErrorUnionImpl(E error) : ErrorUnionImpl<N + 1, Es...>(N, static_cast<usize>(error)) {}

    template <SameAs<E>>
    Optional<E> as() const {
        return ErrorUnionBase::type() == N ? static_cast<E>(ErrorUnionBase::error()) : Optional<E>();
    }
    template <typename F>
    Optional<F> as() const {
        return this->ErrorUnionImpl<N + 1, Es...>::template as<F>();
    }
};

template <typename... Es>
struct ErrorUnion : ErrorUnionImpl<0, Es...> {
    using ErrorUnionImpl<0, Es...>::ErrorUnionImpl;

    template <typename E>
    static constexpr bool contains() {
        return (IsSame<E, Es> || ...);
    }
};

template <typename E, typename F>
concept ErrorUnionContains = E::template contains<F>();

template <typename T, typename E>
class [[nodiscard]] Result {
    union {
        E m_error;
        alignas(T) Array<uint8, sizeof(T)> m_value;
    };
    bool m_is_error;

public:
    Result(E error) : m_error(error), m_is_error(true) {}
    template <typename F>
    Result(F error) requires ErrorUnionContains<E, F> : m_error(error), m_is_error(true) {}
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
    T &value() {
        ASSERT(!m_is_error);
        return *reinterpret_cast<T *>(m_value.data());
    }
    T disown_value() {
        auto disowned = move(value());
        m_is_error = true;
        return disowned;
    }
};

template <typename E>
class [[nodiscard]] Result<void, E> {
    E m_error{};
    const bool m_is_error{false};

public:
    Result() = default;
    Result(E error) : m_error(error), m_is_error(true) {}
    template <typename F>
    Result(F error) requires ErrorUnionContains<E, F> : m_error(error), m_is_error(true) {}
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
    void disown_value() const {}
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
