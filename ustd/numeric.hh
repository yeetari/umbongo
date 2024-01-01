#pragma once

#include <ustd/assert.hh>
#include <ustd/types.hh>

namespace ustd {

template <typename>
inline constexpr bool is_integral = false;
template <>
inline constexpr bool is_integral<bool> = true;
template <>
inline constexpr bool is_integral<char> = true;
template <>
inline constexpr bool is_integral<int8_t> = true;
template <>
inline constexpr bool is_integral<int16_t> = true;
template <>
inline constexpr bool is_integral<int32_t> = true;
template <>
inline constexpr bool is_integral<int64_t> = true;
template <>
inline constexpr bool is_integral<uint8_t> = true;
template <>
inline constexpr bool is_integral<uint16_t> = true;
template <>
inline constexpr bool is_integral<uint32_t> = true;
template <>
inline constexpr bool is_integral<uint64_t> = true;

template <typename T>
inline constexpr bool is_signed = T(-1) < T(0);

template <typename T>
concept Integral = is_integral<T>;

template <typename T>
concept SignedIntegral = is_integral<T> && is_signed<T>;

template <typename T>
concept UnsignedIntegral = is_integral<T> && !is_signed<T>;

template <typename T>
struct Limits;

template <>
struct Limits<bool> {
    static constexpr bool min() { return false; }
    static constexpr bool max() { return true; }
};

template <>
struct Limits<int8_t> {
    static constexpr int8_t min() { return -__INT8_MAX__ - 1; }
    static constexpr int8_t max() { return __INT8_MAX__; }
};

template <>
struct Limits<int16_t> {
    static constexpr int16_t min() { return -__INT16_MAX__ - 1; }
    static constexpr int16_t max() { return __INT16_MAX__; }
};

template <>
struct Limits<int32_t> {
    static constexpr int32_t min() { return -__INT32_MAX__ - 1; }
    static constexpr int32_t max() { return __INT32_MAX__; }
};

template <>
struct Limits<int64_t> {
    static constexpr int64_t min() { return -__INT64_MAX__ - 1; }
    static constexpr int64_t max() { return __INT64_MAX__; }
};

template <>
struct Limits<uint8_t> {
    static constexpr uint8_t min() { return 0; }
    static constexpr uint8_t max() { return __UINT8_MAX__; }
};

template <>
struct Limits<uint16_t> {
    static constexpr uint16_t min() { return 0; }
    static constexpr uint16_t max() { return __UINT16_MAX__; }
};

template <>
struct Limits<uint32_t> {
    static constexpr uint32_t min() { return 0; }
    static constexpr uint32_t max() { return __UINT32_MAX__; }
};

template <>
struct Limits<uint64_t> {
    static constexpr uint64_t min() { return 0; }
    static constexpr uint64_t max() { return __UINT64_MAX__; }
};

template <typename T>
constexpr T ceil_div(T x, T y) {
    return x / y + T(x % y != 0);
}

template <typename T>
constexpr T min(T a, T b) {
    return b < a ? b : a;
}

template <typename T>
constexpr T max(T a, T b) {
    return a < b ? b : a;
}

template <typename T, typename U>
constexpr T align_up(T value, U alignment) {
    static_assert(sizeof(T) >= sizeof(U));
    ASSERT((T(alignment) & T(alignment - 1)) == 0, "Alignment not a power of two");
    return (value + T(alignment) - 1) & ~(T(alignment) - 1);
}

template <typename T, typename U>
constexpr T align_down(T value, U alignment) {
    static_assert(sizeof(T) >= sizeof(U));
    ASSERT((T(alignment) & T(alignment - 1)) == 0, "Alignment not a power of two");
    return value & ~(T(alignment) - 1);
}

// NOLINTBEGIN(google-runtime-int)
template <UnsignedIntegral T>
constexpr T popcount(T value) {
    if constexpr (sizeof(T) <= sizeof(unsigned int)) {
        return __builtin_popcount(static_cast<unsigned int>(value));
    } else if constexpr (sizeof(T) <= sizeof(unsigned long)) {
        return __builtin_popcountl(static_cast<unsigned long>(value));
    } else if constexpr (sizeof(T) <= sizeof(unsigned long long)) {
        return __builtin_popcountll(static_cast<unsigned long long>(value));
    }
}

template <UnsignedIntegral T>
constexpr T clz(T value) {
    constexpr int bit_count = 8 * sizeof(T);
    if (value == 0) {
        return T(bit_count);
    }
    if constexpr (sizeof(T) <= sizeof(unsigned int)) {
        return T(__builtin_clz(value)) - (8 * sizeof(unsigned int) - bit_count);
    } else if constexpr (sizeof(T) <= sizeof(unsigned long)) {
        return T(__builtin_clzl(value)) - (8 * sizeof(unsigned long) - bit_count);
    } else if constexpr (sizeof(T) <= sizeof(unsigned long long)) {
        return T(__builtin_clzll(value)) - (8 * sizeof(unsigned long long) - bit_count);
    }
}

template <UnsignedIntegral T>
constexpr T ctz(T value) {
    constexpr int bit_count = 8 * sizeof(T);
    if (value == 0) {
        return T(bit_count);
    }
    if constexpr (sizeof(T) <= sizeof(unsigned int)) {
        return T(__builtin_ctz(value));
    } else if constexpr (sizeof(T) <= sizeof(unsigned long)) {
        return T(__builtin_ctzl(value));
    } else if constexpr (sizeof(T) <= sizeof(unsigned long long)) {
        return T(__builtin_ctzll(value));
    }
}
// NOLINTEND(google-runtime-int)

constexpr uint8_t decode_bcd(uint8_t byte) {
    return ((byte & 0xf0u) >> 4u) * 10 + (byte & 0xfu);
}

} // namespace ustd
