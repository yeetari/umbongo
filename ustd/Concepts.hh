#pragma once

#include <ustd/Traits.hh>

namespace ustd {

template <typename T, typename U>
concept ConvertibleTo = is_convertible_to<T, U>;

template <typename T>
concept Enum = is_enum<T>;

template <typename T>
concept Integral = is_integral<T>;

template <typename T>
concept Pointer = is_pointer<T>;

template <typename T, typename U>
concept SameAs = is_same<T, U>;

template <typename T>
concept SignedIntegral = is_integral<T> && is_signed<T>;

template <typename T>
concept TriviallyCopyable = is_trivially_copyable<T>;

template <typename T>
concept UnsignedIntegral = is_integral<T> && !is_signed<T>;

} // namespace ustd
