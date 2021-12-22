#pragma once

#include <core/Error.hh>
#include <kernel/Syscall.hh>      // IWYU pragma: export
#include <kernel/SyscallTypes.hh> // IWYU pragma: export
#include <ustd/Result.hh>
#include <ustd/Types.hh>

namespace core {

template <typename R = usize>
inline ustd::Result<R, SysError> syscall(Syscall::Number number) {
    ssize result; // NOLINT
    asm volatile("syscall" : "=a"(result) : "a"(number) : "rcx", "r11", "memory");
    if (result < 0) {
        return static_cast<SysError>(result);
    }
    return R(result);
}

template <typename R = usize, typename T>
inline ustd::Result<R, SysError> syscall(Syscall::Number number, const T &rdi) {
    ssize result;                                                                            // NOLINT
    asm volatile("syscall" : "=a"(result) : "a"(number), "D"(rdi) : "rcx", "r11", "memory"); // NOLINT
    if (result < 0) {
        return static_cast<SysError>(result);
    }
    return R(result);
}

template <typename R = usize, typename T, typename U>
inline ustd::Result<R, SysError> syscall(Syscall::Number number, const T &rdi, const U &rsi) {
    ssize result;                                                                                      // NOLINT
    asm volatile("syscall" : "=a"(result) : "a"(number), "D"(rdi), "S"(rsi) : "rcx", "r11", "memory"); // NOLINT
    if (result < 0) {
        return static_cast<SysError>(result);
    }
    return R(result);
}

template <typename R = usize, typename T, typename U, typename V>
inline ustd::Result<R, SysError> syscall(Syscall::Number number, const T &rdi, const U &rsi, const V &rdx) {
    ssize result; // NOLINT
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
