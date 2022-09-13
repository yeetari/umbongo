#pragma once

#include <ustd/Types.hh>
#include <ustd/Utility.hh>

namespace ustd {

template <typename T, T V>
struct IntegralConstant {
    static constexpr T value = V;
};

template <bool B>
using BoolConstant = IntegralConstant<bool, B>;
using FalseType = BoolConstant<false>;
using TrueType = BoolConstant<true>;

namespace detail {

template <bool B, typename T, typename F>
struct Conditional {
    using type = T;
};
template <typename T, typename F>
struct Conditional<false, T, F> {
    using type = F;
};

} // namespace detail

template <bool B, typename T, typename F>
using conditional = typename detail::Conditional<B, T, F>::type;

template <typename>
inline constexpr bool is_const = false;
template <typename T>
inline constexpr bool is_const<const T> = true;

template <typename T>
inline constexpr bool is_enum = __is_enum(T);

template <typename>
inline constexpr bool is_integral = false;
template <>
inline constexpr bool is_integral<bool> = true;
template <>
inline constexpr bool is_integral<char> = true;
template <>
inline constexpr bool is_integral<int8_t> = true;
template <>
inline constexpr bool is_integral<int16_t> = true;
template <>
inline constexpr bool is_integral<int32_t> = true;
template <>
inline constexpr bool is_integral<int64_t> = true;
template <>
inline constexpr bool is_integral<uint8_t> = true;
template <>
inline constexpr bool is_integral<uint16_t> = true;
template <>
inline constexpr bool is_integral<uint32_t> = true;
template <>
inline constexpr bool is_integral<uint64_t> = true;

template <typename T>
inline constexpr bool is_move_constructible = requires(T &&t) {
    new T(move(t));
};

template <typename>
inline constexpr bool is_pointer = false;
template <typename T>
inline constexpr bool is_pointer<T *> = true;

template <typename>
inline constexpr bool is_reference = false;
template <typename T>
inline constexpr bool is_reference<T &> = true;
template <typename T>
inline constexpr bool is_reference<T &&> = true;

template <typename, typename>
inline constexpr bool is_same = false;
template <typename T>
inline constexpr bool is_same<T, T> = true;

template <typename T>
inline constexpr bool is_signed = T(-1) < T(0);

template <typename T>
inline constexpr bool is_trivially_constructible = __is_trivially_constructible(T);
template <typename T>
inline constexpr bool is_trivially_copyable = __is_trivially_copyable(T);
template <typename T>
inline constexpr bool is_trivially_destructible = __is_trivially_destructible(T);

template <typename T, typename U>
inline constexpr bool is_convertible_to = is_same<T, U> || requires(T obj) {
    static_cast<U>(obj);
};

template <typename E>
using underlying_type = __underlying_type(E);

template <typename E>
constexpr auto to_underlying(E value) {
    return static_cast<underlying_type<E>>(value);
}

} // namespace ustd
