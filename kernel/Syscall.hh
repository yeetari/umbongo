#pragma once

#include <ustd/Types.hh>

namespace Syscall {

#define ENUMERATE_SYSCALLS(S) S(print)

enum Number {
#define ENUMERATE_SYSCALL(s) s,
    ENUMERATE_SYSCALLS(ENUMERATE_SYSCALL)
#undef ENUMERATE_SYSCALL
        __Count__,
};

inline usize invoke(Number number) {
    // NOLINTNEXTLINE
    usize result;
    asm volatile("syscall" : "=a"(result) : "a"(number) : "memory");
    return result;
}

template <typename T>
inline usize invoke(Number number, const T &rdi) {
    // NOLINTNEXTLINE
    usize result;
    asm volatile("syscall" : "=a"(result) : "a"(number), "D"(rdi) : "memory");
    return result;
}

template <typename T, typename U>
inline usize invoke(Number number, const T &rdi, const U &rsi) {
    // NOLINTNEXTLINE
    usize result;
    asm volatile("syscall" : "=a"(result) : "a"(number), "D"(rdi), "S"(rsi) : "memory");
    return result;
}

template <typename T, typename U, typename V>
inline usize invoke(Number number, const T &rdi, const U &rsi, const V &rdx) {
    // NOLINTNEXTLINE
    usize result;
    asm volatile("syscall" : "=a"(result) : "a"(number), "D"(rdi), "S"(rsi), "d"(rdx) : "memory");
    return result;
}

} // namespace Syscall
