#pragma once

namespace ustd {
namespace detail {

template <typename T>
struct RemoveCvImpl {
    using type = T;
};
template <typename T>
struct RemoveCvImpl<const T> {
    using type = T;
};
template <typename T>
struct RemoveCvImpl<volatile T> {
    using type = T;
};
template <typename T>
struct RemoveCvImpl<const volatile T> {
    using type = T;
};

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

} // namespace detail

template <typename T>
using remove_cv = typename detail::RemoveCvImpl<T>::type;
template <typename T>
using remove_ref = typename detail::RemoveRefImpl<T>::type;

template <typename T>
T declval();

template <typename T>
constexpr T &&forward(remove_ref<T> &arg) {
    return static_cast<T &&>(arg);
}

template <typename T>
constexpr T &&forward(remove_ref<T> &&arg) {
    return static_cast<T &&>(arg);
}

template <typename T>
constexpr remove_ref<T> &&move(T &&arg) {
    return static_cast<remove_ref<T> &&>(arg);
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
