#pragma once

#include <ustd/Assert.hh>
#include <ustd/Types.hh>

namespace ustd {

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

} // namespace ustd
