#pragma once

#include <ustd/int_types.h> // IWYU pragma: export
// IWYU pragma: no_include "ustd/int_types.h"

namespace std {

enum class align_val_t : size_t {};

} // namespace std

namespace ustd {

using align_val_t = std::align_val_t;

} // namespace ustd

// NOLINTNEXTLINE: Must use unsigned long long here.
constexpr size_t operator"" _KiB(unsigned long long num) {
    return num * 0x400;
}

// NOLINTNEXTLINE: Must use unsigned long long here.
constexpr size_t operator"" _MiB(unsigned long long num) {
    return num * 0x100000;
}

// NOLINTNEXTLINE: Must use unsigned long long here.
constexpr size_t operator"" _GiB(unsigned long long num) {
    return num * 0x40000000;
}

// NOLINTNEXTLINE: Must use unsigned long long here.
constexpr size_t operator"" _TiB(unsigned long long num) {
    return num * 0x10000000000;
}

inline constexpr auto &operator&=(auto &lhs, auto rhs) {
    return lhs = (lhs & rhs);
}
inline constexpr auto &operator|=(auto &lhs, auto rhs) {
    return lhs = (lhs | rhs);
}
inline constexpr auto &operator^=(auto &lhs, auto rhs) {
    return lhs = (lhs ^ rhs);
}
