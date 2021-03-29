#pragma once

namespace ustd {

template <typename T>
constexpr const T &max(const T &a, const T &b) {
    return a < b ? b : a;
}

template <typename T>
constexpr const T &min(const T &a, const T &b) {
    return b < a ? b : a;
}

} // namespace ustd
