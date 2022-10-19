#pragma once

namespace ustd {
namespace detail {

template <typename T>
struct RemoveCv {
    using type = T;
};
template <typename T>
struct RemoveCv<const T> {
    using type = T;
};
template <typename T>
struct RemoveCv<volatile T> {
    using type = T;
};
template <typename T>
struct RemoveCv<const volatile T> {
    using type = T;
};

template <typename T>
struct RemoveRef {
    using type = T;
};
template <typename T>
struct RemoveRef<T &> {
    using type = T;
};
template <typename T>
struct RemoveRef<T &&> {
    using type = T;
};

} // namespace detail

template <typename T>
using remove_cv = typename detail::RemoveCv<T>::type;
template <typename T>
using remove_ref = typename detail::RemoveRef<T>::type;

// TODO: Actually do the handling for arrays and functions.
template <typename T>
using decay = remove_cv<remove_ref<T>>;

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

inline void *operator new(__SIZE_TYPE__, void *ptr) {
    return ptr;
}

inline void *operator new[](__SIZE_TYPE__, void *ptr) {
    return ptr;
}
