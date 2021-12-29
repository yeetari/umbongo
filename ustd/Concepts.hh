#pragma once

#include <ustd/Traits.hh>

namespace ustd {

template <typename T, typename U>
concept ConvertibleTo = IsConvertibleTo<T, U>;

template <typename T>
concept Integral = IsIntegral<T>;

template <typename T>
concept Pointer = IsPointer<T>;

template <typename T, typename U>
concept SameAs = IsSame<T, U>;

template <typename T>
concept SignedIntegral = IsIntegral<T> && IsSigned<T>;

template <typename T>
concept TriviallyCopyable = IsTriviallyCopyable<T>;

template <typename T>
concept UnsignedIntegral = IsIntegral<T> && !IsSigned<T>;

} // namespace ustd
