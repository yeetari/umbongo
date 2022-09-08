#pragma once

#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/Concepts.hh>
#include <ustd/Optional.hh>
#include <ustd/Traits.hh>
#include <ustd/Utility.hh>

namespace ustd {

class ErrorUnionBase {
private:
    size_t m_type;
    size_t m_error;

public:
    ErrorUnionBase() = default;
    ErrorUnionBase(size_t type, size_t error) : m_type(type), m_error(error) {}

    size_t error() const { return m_error; }
    size_t type() const { return m_type; }
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

    ErrorUnionImpl(E error) : ErrorUnionImpl<N + 1, Es...>(N, static_cast<size_t>(error)) {}

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
        return (is_same<E, Es> || ...);
    }
};

template <typename E, typename F>
concept ErrorUnionContains = E::template contains<F>();

template <typename T, typename E>
class [[nodiscard]] Result {
    union {
        E m_error;
        alignas(T) Array<uint8_t, sizeof(T)> m_value;
    };
    bool m_is_error{true};

public:
    Result(E error) : m_error(error) {}
    template <typename F>
    Result(F error) requires ErrorUnionContains<E, F> : m_error(error) {}
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

    bool has_value() const { return !m_is_error; }
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

    bool has_value() const { return !m_is_error; }
    bool is_error() const { return m_is_error; }
    E error() const {
        ASSERT(m_is_error);
        return m_error;
    }
    void value() const {}
    int disown_value() const { return 0; }
};

template <typename T>
class RefWrapper {
    T *m_ptr;

public:
    static constexpr bool k_is_ref_wrapper = true;
    RefWrapper(T &ptr) : m_ptr(&ptr) {}
    T &ref() { return *m_ptr; }
};

template <typename T, typename E>
class [[nodiscard]] Result<T &, E> {
    union {
        T *const m_ptr{nullptr};
        E m_error;
    };
    const bool m_is_error{true};

public:
    Result(E error) : m_error(error) {}
    template <typename F>
    Result(F error) requires ErrorUnionContains<E, F> : m_error(error) {}
    Result(T &ref) : m_ptr(&ref), m_is_error(false) {}
    Result(const Result &) = delete;
    Result(Result &&) = delete;
    ~Result() = default;

    Result &operator=(const Result &) = delete;
    Result &operator=(Result &&) = delete;

    bool operator==(E error) const { return m_is_error && m_error == error; }

    bool has_value() const { return !m_is_error; }
    bool is_error() const { return m_is_error; }
    E error() const {
        ASSERT(m_is_error);
        return m_error;
    }
    T &value() {
        ASSERT(!m_is_error);
        return *m_ptr;
    }
    RefWrapper<T> disown_value() { return value(); }
};

template <typename T, typename E>
Result<T, E>::~Result() {
    if constexpr (!is_trivially_destructible<T>) {
        if (!m_is_error) {
            reinterpret_cast<T *>(m_value.data())->~T();
        }
    }
}

} // namespace ustd
