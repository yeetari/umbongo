#pragma once

#include <ustd/Concepts.hh>
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
constexpr T ceil_div(T a, T b) {
    return a / b + (a % b != 0);
}

template <typename T>
constexpr T min(T a, T b) {
    return b < a ? b : a;
}

template <typename T>
constexpr T max(T a, T b) {
    return a < b ? b : a;
}

template <typename T>
constexpr T clamp(T val, T min_val, T max_val) {
    return min(max(val, min_val), max_val);
}

constexpr usize round_down(usize roundee, usize roundend) {
    return roundee & ~(roundend - 1);
}

constexpr usize round_up(usize roundee, usize roundend) {
    return (roundee + roundend - 1) & ~(roundend - 1);
}

template <UnsignedIntegral T>
constexpr T clz(T value) {
    constexpr int bit_count = 8 * sizeof(T);
    if (value == 0) {
        return bit_count;
    }
    if constexpr (sizeof(T) <= sizeof(unsigned)) {
        return __builtin_clz(value) - (8 * sizeof(unsigned) - bit_count);
    } else if constexpr (sizeof(T) <= sizeof(unsigned long)) {
        return __builtin_clzl(value) - (8 * sizeof(unsigned long) - bit_count);
    } else if constexpr (sizeof(T) <= sizeof(unsigned long long)) {
        return __builtin_clzll(value) - (8 * sizeof(unsigned long long) - bit_count);
    } else {
        static_assert(!IsSame<T, T>);
    }
}

template <UnsignedIntegral T>
constexpr T ctz(T value) {
    if (value == 0) {
        return 8 * sizeof(T);
    }
    if constexpr (sizeof(T) <= sizeof(unsigned)) {
        return __builtin_ctz(value);
    } else if constexpr (sizeof(T) <= sizeof(unsigned long)) {
        return __builtin_ctzl(value);
    } else if constexpr (sizeof(T) <= sizeof(unsigned long long)) {
        return __builtin_ctzll(value);
    } else {
        static_assert(!IsSame<T, T>);
    }
}

template <UnsignedIntegral T>
constexpr T clo(T value) {
    return value != Limits<T>::max() ? clz(~value) : 8 * sizeof(T);
}

template <UnsignedIntegral T>
constexpr T cto(T value) {
    return value != Limits<T>::max() ? ctz(~value) : 8 * sizeof(T);
}

template <UnsignedIntegral T>
constexpr T popcount(T value) {
    if constexpr (sizeof(T) <= sizeof(unsigned)) {
        return __builtin_popcount(value);
    } else if constexpr (sizeof(T) <= sizeof(unsigned long)) {
        return __builtin_popcountl(value);
    } else if constexpr (sizeof(T) <= sizeof(unsigned long long)) {
        return __builtin_popcountll(value);
    } else {
        static_assert(!IsSame<T, T>);
    }
}

} // namespace ustd
