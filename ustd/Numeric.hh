#pragma once

#include <ustd/Types.hh>

namespace ustd {

template <typename T>
struct Limits {};

template <>
struct Limits<usize> {
    static constexpr usize min() { return -__SIZE_MAX__ - 1; }
    static constexpr usize max() { return __SIZE_MAX__; }
};

template <typename T>
constexpr const T &max(const T &a, const T &b) {
    return a < b ? b : a;
}

template <typename T>
constexpr const T &min(const T &a, const T &b) {
    return b < a ? b : a;
}

} // namespace ustd

using ustd::Limits;
