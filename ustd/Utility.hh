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
constexpr T &&forward(RemoveRef<T> &arg) noexcept {
    return static_cast<T &&>(arg);
}

template <typename T>
constexpr T &&forward(RemoveRef<T> &&arg) noexcept {
    return static_cast<T &&>(arg);
}

template <typename T>
constexpr RemoveRef<T> &&move(T &&arg) noexcept {
    return static_cast<RemoveRef<T> &&>(arg);
}

} // namespace ustd
