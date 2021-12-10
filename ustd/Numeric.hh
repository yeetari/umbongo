#pragma once

#include <ustd/Types.hh>

namespace ustd {

template <typename T>
struct Limits {};

template <>
struct Limits<bool> {
    static constexpr bool min() { return false; }
    static constexpr bool max() { return true; }
};

template <>
struct Limits<int8> {
    static constexpr int8 min() { return -__INT8_MAX__ - 1; }
    static constexpr int8 max() { return __INT8_MAX__; }
};

template <>
struct Limits<int16> {
    static constexpr int16 min() { return -__INT16_MAX__ - 1; }
    static constexpr int16 max() { return __INT16_MAX__; }
};

template <>
struct Limits<int32> {
    static constexpr int32 min() { return -__INT32_MAX__ - 1; }
    static constexpr int32 max() { return __INT32_MAX__; }
};

template <>
struct Limits<int64> {
    static constexpr int64 min() { return -__INT64_MAX__ - 1; }
    static constexpr int64 max() { return __INT64_MAX__; }
};

template <>
struct Limits<uint8> {
    static constexpr uint8 min() { return 0; }
    static constexpr uint8 max() { return __UINT8_MAX__; }
};

template <>
struct Limits<uint16> {
    static constexpr uint16 min() { return 0; }
    static constexpr uint16 max() { return __UINT16_MAX__; }
};

template <>
struct Limits<uint32> {
    static constexpr uint32 min() { return 0; }
    static constexpr uint32 max() { return __UINT32_MAX__; }
};

template <>
struct Limits<uint64> {
    static constexpr uint64 min() { return 0; }
    static constexpr uint64 max() { return __UINT64_MAX__; }
};

template <typename T>
constexpr const T &max(const T &a, const T &b) {
    return a < b ? b : a;
}

template <typename T>
constexpr const T &min(const T &a, const T &b) {
    return b < a ? b : a;
}

constexpr usize round_down(usize roundee, usize roundend) {
    return roundee & ~(roundend - 1);
}

constexpr usize round_up(usize roundee, usize roundend) {
    return (roundee + roundend - 1) & ~(roundend - 1);
}

} // namespace ustd
