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
struct IsIntegerCheck : public FalseType {};
template <>
struct IsIntegerCheck<bool> : public TrueType {};
template <>
struct IsIntegerCheck<char> : public TrueType {};
template <>
struct IsIntegerCheck<int8> : public TrueType {};
template <>
struct IsIntegerCheck<int16> : public TrueType {};
template <>
struct IsIntegerCheck<int32> : public TrueType {};
template <>
struct IsIntegerCheck<int64> : public TrueType {};
template <>
struct IsIntegerCheck<uint8> : public TrueType {};
template <>
struct IsIntegerCheck<uint16> : public TrueType {};
template <>
struct IsIntegerCheck<uint32> : public TrueType {};
template <>
struct IsIntegerCheck<uint64> : public TrueType {};

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
concept IsConvertibleTo = requires(T obj) {
    static_cast<U>(obj);
};

template <typename T>
concept IsInteger = detail::IsIntegerCheck<RemoveQual<T>>::value;

template <typename T>
concept IsPointer = detail::IsPointerCheck<RemoveQual<T>>::value;

template <typename T, typename U>
concept IsSame = detail::IsSameCheck<T, U>::value;

template <typename T>
concept IsTriviallyCopyable = __is_trivially_copyable(T);

} // namespace ustd

using ustd::Conditional;
using ustd::IsConvertibleTo;
using ustd::IsInteger;
using ustd::IsPointer;
using ustd::IsSame;
using ustd::IsTriviallyCopyable;
