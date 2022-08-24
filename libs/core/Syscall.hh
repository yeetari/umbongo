#pragma once

#include <core/Error.hh>
#include <kernel/Syscall.hh>      // IWYU pragma: export
#include <kernel/SyscallTypes.hh> // IWYU pragma: export
#include <ustd/Result.hh>
#include <ustd/Types.hh>

namespace core {

template <typename R = size_t>
inline ustd::Result<R, SysError> syscall(Syscall::Number number) {
    ssize_t result; // NOLINT
    asm volatile("syscall" : "=a"(result) : "a"(number) : "rcx", "r11", "memory");
    if (result < 0) {
        return static_cast<SysError>(result);
    }
    return R(result);
}

template <typename R = size_t, typename T>
inline ustd::Result<R, SysError> syscall(Syscall::Number number, const T &rdi) {
    ssize_t result;                                                                          // NOLINT
    asm volatile("syscall" : "=a"(result) : "a"(number), "D"(rdi) : "rcx", "r11", "memory"); // NOLINT
    if (result < 0) {
        return static_cast<SysError>(result);
    }
    return R(result);
}

template <typename R = size_t, typename T, typename U>
inline ustd::Result<R, SysError> syscall(Syscall::Number number, const T &rdi, const U &rsi) {
    ssize_t result;                                                                                    // NOLINT
    asm volatile("syscall" : "=a"(result) : "a"(number), "D"(rdi), "S"(rsi) : "rcx", "r11", "memory"); // NOLINT
    if (result < 0) {
        return static_cast<SysError>(result);
    }
    return R(result);
}

template <typename R = size_t, typename T, typename U, typename V>
inline ustd::Result<R, SysError> syscall(Syscall::Number number, const T &rdi, const U &rsi, const V &rdx) {
    ssize_t result; // NOLINT
    asm volatile("syscall"
                 : "=a"(result)
                 : "a"(number), "D"(rdi), "S"(rsi), "d"(rdx)
                 : "rcx", "r11", "memory"); // NOLINT
    if (result < 0) {
        return static_cast<SysError>(result);
    }
    return R(result);
}

} // namespace core
