#pragma once

#include <ustd/Traits.hh>

namespace ustd {

template <typename T, typename U>
concept ConvertibleTo = IsConvertibleTo<T, U>;

template <typename T>
concept Integral = IsIntegral<T>;

template <typename T, typename U>
concept SameAs = IsSame<T, U>;

} // namespace ustd

using ustd::ConvertibleTo;
using ustd::Integral;
using ustd::SameAs;
