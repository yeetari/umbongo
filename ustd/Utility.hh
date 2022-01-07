#pragma once

namespace ustd {
namespace detail {

template <typename T>
struct RemoveRefImpl {
    using type = T;
};
template <typename T>
struct RemoveRefImpl<T &> {
    using type = T;
};
template <typename T>
struct RemoveRefImpl<T &&> {
    using type = T;
};

template <typename T>
struct RemoveQualImpl {
    using type = T;
};
template <typename T>
struct RemoveQualImpl<const T> {
    using type = T;
};
template <typename T>
struct RemoveQualImpl<volatile T> {
    using type = T;
};
template <typename T>
struct RemoveQualImpl<const volatile T> {
    using type = T;
};

} // namespace detail

template <typename T>
using RemoveRef = typename detail::RemoveRefImpl<T>::type;

template <typename T>
using RemoveQual = typename detail::RemoveQualImpl<T>::type;

template <typename T>
T declval();

template <typename T>
constexpr T &&forward(RemoveRef<T> &arg) {
    return static_cast<T &&>(arg);
}

template <typename T>
constexpr T &&forward(RemoveRef<T> &&arg) {
    return static_cast<T &&>(arg);
}

template <typename T>
constexpr RemoveRef<T> &&move(T &&arg) {
    return static_cast<RemoveRef<T> &&>(arg);
}

template <typename T, typename U = T>
constexpr T exchange(T &obj, U &&new_value) {
    T old_value = move(obj);
    obj = forward<U>(new_value);
    return old_value;
}

template <typename T>
constexpr void swap(T &lhs, T &rhs) {
    T tmp(move(lhs));
    lhs = move(rhs);
    rhs = move(tmp);
}

} // namespace ustd
