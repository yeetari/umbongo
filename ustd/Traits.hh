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

template <typename T>
struct IsPointerCheck : public FalseType {};
template <typename T>
struct IsPointerCheck<T *> : public TrueType {};

template <typename, typename>
struct IsSameCheck : public FalseType {};
template <typename T>
struct IsSameCheck<T, T> : public TrueType {};

} // namespace detail

template <bool B, typename T, typename F>
using Conditional = typename detail::Conditional<B, T, F>::type;

template <typename T, typename U>
inline constexpr bool IsConvertibleTo = detail::IsSameCheck<T, U>::value || requires(T obj) {
    static_cast<U>(obj);
};

template <typename T>
inline constexpr bool IsIntegral = detail::IsIntegralCheck<RemoveQual<T>>::value;

template <typename T>
inline constexpr bool IsMoveConstructible = requires(T &&t) {
    new T(move(t));
};

template <typename T>
inline constexpr bool IsPointer = detail::IsPointerCheck<RemoveQual<T>>::value;

template <typename T, typename U>
inline constexpr bool IsSame = detail::IsSameCheck<T, U>::value;

template <typename T>
concept IsTriviallyCopyable = __is_trivially_copyable(T);
template <typename T>
concept IsTriviallyDestructible = __is_trivially_destructible(T);

} // namespace ustd
