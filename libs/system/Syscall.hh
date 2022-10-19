#pragma once

#include <system/Syscalls.h>
#include <system/Types.h>
#include <ustd/Result.hh>
#include <ustd/Types.hh>

namespace system {

// NOLINTBEGIN(*-init-variables)
template <typename R = size_t>
ustd::Result<R, ub_error_t> syscall(ub_syscall_t number) {
    ssize_t result;
    asm volatile("syscall" : "=a"(result) : "a"(number) : "rcx", "r11", "memory");
    if (result < 0) [[unlikely]] {
        return static_cast<ub_error_t>(result);
    }
    return R(result);
}

template <typename R = size_t, typename T>
ustd::Result<R, ub_error_t> syscall(ub_syscall_t number, T a1) {
    ssize_t result;
    asm volatile("syscall" : "=a"(result) : "a"(number), "D"(a1) : "rcx", "r11", "memory");
    if (result < 0) [[unlikely]] {
        return static_cast<ub_error_t>(result);
    }
    return R(result);
}

template <typename R = size_t, typename T, typename U>
ustd::Result<R, ub_error_t> syscall(ub_syscall_t number, T a1, U a2) {
    ssize_t result;
    asm volatile("syscall" : "=a"(result) : "a"(number), "D"(a1), "S"(a2) : "rcx", "r11", "memory");
    if (result < 0) [[unlikely]] {
        return static_cast<ub_error_t>(result);
    }
    return R(result);
}

template <typename R = size_t, typename T, typename U, typename V>
ustd::Result<R, ub_error_t> syscall(ub_syscall_t number, T a1, U a2, V a3) {
    ssize_t result;
    asm volatile("syscall" : "=a"(result) : "a"(number), "D"(a1), "S"(a2), "d"(a3) : "rcx", "r11", "memory");
    if (result < 0) [[unlikely]] {
        return static_cast<ub_error_t>(result);
    }
    return R(result);
}

template <typename R = size_t, typename T, typename U, typename V, typename W>
ustd::Result<R, ub_error_t> syscall(ub_syscall_t number, T a1, U a2, V a3, W a4) {
    register W r10 asm("r10") = a4;
    ssize_t result;
    asm volatile("syscall" : "=a"(result) : "a"(number), "D"(a1), "S"(a2), "d"(a3), "r"(r10) : "rcx", "r11", "memory");
    if (result < 0) [[unlikely]] {
        return static_cast<ub_error_t>(result);
    }
    return R(result);
}

template <typename R = size_t, typename T, typename U, typename V, typename W, typename X>
ustd::Result<R, ub_error_t> syscall(ub_syscall_t number, T a1, U a2, V a3, W a4, X a5) {
    register W r10 asm("r10") = a4;
    register X r8 asm("r8") = a5;
    ssize_t result;
    asm volatile("syscall"
                 : "=a"(result)
                 : "a"(number), "D"(a1), "S"(a2), "d"(a3), "r"(r10), "r"(r8)
                 : "rcx", "r11", "memory");
    if (result < 0) [[unlikely]] {
        return static_cast<ub_error_t>(result);
    }
    return R(result);
}

template <typename R = size_t, typename T, typename U, typename V, typename W, typename X, typename Y>
ustd::Result<R, ub_error_t> syscall(ub_syscall_t number, T a1, U a2, V a3, W a4, X a5, Y a6) {
    register W r10 asm("r10") = a4;
    register X r8 asm("r8") = a5;
    register X r9 asm("r9") = a6;
    ssize_t result;
    asm volatile("syscall"
                 : "=a"(result)
                 : "a"(number), "D"(a1), "S"(a2), "d"(a3), "r"(r10), "r"(r8), "r"(r9)
                 : "rcx", "r11", "memory");
    if (result < 0) [[unlikely]] {
        return static_cast<ub_error_t>(result);
    }
    return R(result);
}
// NOLINTEND(*-init-variables)

} // namespace system
