#pragma once

#include <system/error.h>
#include <system/system.h>
#include <ustd/result.hh>
#include <ustd/types.hh>

namespace system {

// NOLINTBEGIN(*-init-variables)
template <typename R = size_t>
inline ustd::Result<R, ub_error_t> syscall(ub_syscall_t number) {
    ssize_t result;
    asm volatile("syscall" : "=a"(result) : "a"(number) : "rcx", "r11", "memory");
    if (result < 0) [[unlikely]] {
        return static_cast<ub_error_t>(result);
    }
    return R(result);
}

template <typename R = size_t, typename A1>
inline ustd::Result<R, ub_error_t> syscall(ub_syscall_t number, A1 a1) {
    ssize_t result;
    asm volatile("syscall" : "=a"(result) : "a"(number), "D"(a1) : "rcx", "r11", "memory");
    if (result < 0) [[unlikely]] {
        return static_cast<ub_error_t>(result);
    }
    return R(result);
}

template <typename R = size_t, typename A1, typename A2>
inline ustd::Result<R, ub_error_t> syscall(ub_syscall_t number, A1 a1, A2 a2) {
    ssize_t result;
    asm volatile("syscall" : "=a"(result) : "a"(number), "D"(a1), "S"(a2) : "rcx", "r11", "memory");
    if (result < 0) [[unlikely]] {
        return static_cast<ub_error_t>(result);
    }
    return R(result);
}

template <typename R = size_t, typename A1, typename A2, typename A3>
inline ustd::Result<R, ub_error_t> syscall(ub_syscall_t number, A1 a1, A2 a2, A3 a3) {
    ssize_t result;
    asm volatile("syscall" : "=a"(result) : "a"(number), "D"(a1), "S"(a2), "d"(a3) : "rcx", "r11", "memory");
    if (result < 0) [[unlikely]] {
        return static_cast<ub_error_t>(result);
    }
    return R(result);
}

template <typename R = size_t, typename A1, typename A2, typename A3, typename A4>
inline ustd::Result<R, ub_error_t> syscall(ub_syscall_t number, A1 a1, A2 a2, A3 a3, A4 a4) {
    register A4 r10 asm("r10") = a4;
    ssize_t result;
    asm volatile("syscall" : "=a"(result) : "a"(number), "D"(a1), "S"(a2), "d"(a3), "r"(r10) : "rcx", "r11", "memory");
    if (result < 0) [[unlikely]] {
        return static_cast<ub_error_t>(result);
    }
    return R(result);
}

template <typename R = size_t, typename A1, typename A2, typename A3, typename A4, typename A5>
inline ustd::Result<R, ub_error_t> syscall(ub_syscall_t number, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) {
    register A4 r10 asm("r10") = a4;
    register A5 r8 asm("r8") = a5;
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

template <typename R = size_t, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
inline ustd::Result<R, ub_error_t> syscall(ub_syscall_t number, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) {
    register A4 r10 asm("r10") = a4;
    register A5 r8 asm("r8") = a5;
    register A6 r9 asm("r9") = a6;
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
