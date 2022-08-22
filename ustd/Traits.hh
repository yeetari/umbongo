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

template <typename>
struct IsConstCheck {
    static constexpr bool value = false;
};
template <typename T>
struct IsConstCheck<const T> {
    static constexpr bool value = true;
};

template <typename>
struct IsIntegralCheck : public FalseType {};
template <>
struct IsIntegralCheck<bool> : public TrueType {};
template <>
struct IsIntegralCheck<char> : public TrueType {};
template <>
struct IsIntegralCheck<int8> : public TrueType {};
template <>
struct IsIntegralCheck<int16> : public TrueType {};
template <>
struct IsIntegralCheck<int32> : public TrueType {};
template <>
struct IsIntegralCheck<int64> : public TrueType {};
template <>
struct IsIntegralCheck<uint8> : public TrueType {};
template <>
struct IsIntegralCheck<uint16> : public TrueType {};
template <>
struct IsIntegralCheck<uint32> : public TrueType {};
template <>
struct IsIntegralCheck<uint64> : public TrueType {};

template <typename>
struct IsPointerCheck : public FalseType {};
template <typename T>
struct IsPointerCheck<T *> : public TrueType {};

template <typename, typename>
struct IsSameCheck : public FalseType {};
template <typename T>
struct IsSameCheck<T, T> : public TrueType {};

} // namespace detail

template <bool B, typename T, typename F>
using conditional = typename detail::Conditional<B, T, F>::type;

template <typename T, typename U>
inline constexpr bool is_convertible_to = detail::IsSameCheck<T, U>::value || requires(T obj) {
    static_cast<U>(obj);
};

template <typename T>
inline constexpr bool is_const = detail::IsConstCheck<T>::value;

template <typename T>
inline constexpr bool is_integral = detail::IsIntegralCheck<remove_cv<T>>::value;

template <typename T>
inline constexpr bool is_move_constructible = requires(T &&t) {
    new T(move(t));
};

template <typename T>
inline constexpr bool is_pointer = detail::IsPointerCheck<remove_cv<T>>::value;

template <typename T, typename U>
inline constexpr bool is_same = detail::IsSameCheck<T, U>::value;

template <typename T>
inline constexpr bool is_signed = T(-1) < T(0);

template <typename T>
concept is_trivially_copyable = __is_trivially_copyable(T);
template <typename T>
concept is_trivially_destructible = __is_trivially_destructible(T);

} // namespace ustd
