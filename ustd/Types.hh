#pragma once

using int8_t = __INT8_TYPE__;
using int16_t = __INT16_TYPE__;
using int32_t = __INT32_TYPE__;
using int64_t = __INT64_TYPE__;
using ptrdiff_t = __PTRDIFF_TYPE__;
using ssize_t = __INTPTR_TYPE__;
using uint8_t = __UINT8_TYPE__;
using uint16_t = __UINT16_TYPE__;
using uint32_t = __UINT32_TYPE__;
using uint64_t = __UINT64_TYPE__;
using uintptr_t = __UINTPTR_TYPE__;
using size_t = __SIZE_TYPE__;

namespace std {

enum class align_val_t : size_t {};

} // namespace std

namespace ustd {

using align_val_t = std::align_val_t;

} // namespace ustd

// NOLINTNEXTLINE: Must used unsigned long long here.
constexpr size_t operator"" _KiB(unsigned long long num) {
    return num * 0x400;
}

// NOLINTNEXTLINE: Must used unsigned long long here.
constexpr size_t operator"" _MiB(unsigned long long num) {
    return num * 0x100000;
}

// NOLINTNEXTLINE: Must used unsigned long long here.
constexpr size_t operator"" _GiB(unsigned long long num) {
    return num * 0x40000000;
}

// NOLINTNEXTLINE: Must used unsigned long long here.
constexpr size_t operator"" _TiB(unsigned long long num) {
    return num * 0x10000000000;
}
