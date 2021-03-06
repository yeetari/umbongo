#pragma once

using int8 = __INT8_TYPE__;
using int16 = __INT16_TYPE__;
using int32 = __INT32_TYPE__;
using int64 = __INT64_TYPE__;
using ptrdiff = __PTRDIFF_TYPE__;
using uint8 = __UINT8_TYPE__;
using uint16 = __UINT16_TYPE__;
using uint32 = __UINT32_TYPE__;
using uint64 = __UINT64_TYPE__;
using uintptr = __UINTPTR_TYPE__;
using usize = __SIZE_TYPE__;

// NOLINTNEXTLINE
constexpr usize operator"" _KiB(unsigned long long num) {
    return num * 0x400;
}

// NOLINTNEXTLINE
constexpr usize operator"" _MiB(unsigned long long num) {
    return num * 0x100000;
}

// NOLINTNEXTLINE
constexpr usize operator"" _GiB(unsigned long long num) {
    return num * 0x40000000;
}

constexpr usize round_up(usize roundee, usize roundend) {
    return (roundee + roundend - 1) & ~(roundend - 1);
}
