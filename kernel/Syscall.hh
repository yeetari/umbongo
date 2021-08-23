#pragma once

#include <ustd/Types.hh>

namespace Syscall {

#define ENUMERATE_SYSCALLS(S)                                                                                          \
    S(allocate_region)                                                                                                 \
    S(chdir)                                                                                                           \
    S(close)                                                                                                           \
    S(create_pipe)                                                                                                     \
    S(create_process)                                                                                                  \
    S(dup_fd)                                                                                                          \
    S(exit)                                                                                                            \
    S(getcwd)                                                                                                          \
    S(getpid)                                                                                                          \
    S(ioctl)                                                                                                           \
    S(mkdir)                                                                                                           \
    S(mmap)                                                                                                            \
    S(mount)                                                                                                           \
    S(open)                                                                                                            \
    S(putchar)                                                                                                         \
    S(read)                                                                                                            \
    S(read_directory)                                                                                                  \
    S(seek)                                                                                                            \
    S(size)                                                                                                            \
    S(wait_pid)                                                                                                        \
    S(write)

enum Number {
#define ENUMERATE_SYSCALL(s) s,
    ENUMERATE_SYSCALLS(ENUMERATE_SYSCALL)
#undef ENUMERATE_SYSCALL
        __Count__,
};

template <typename Ret = ssize>
inline Ret invoke(Number number) {
    Ret result; // NOLINT
    asm volatile("syscall" : "=a"(result) : "a"(number) : "rcx", "r11", "memory");
    return result;
}

template <typename Ret = ssize, typename T>
inline Ret invoke(Number number, const T &rdi) {
    Ret result;                                                                              // NOLINT
    asm volatile("syscall" : "=a"(result) : "a"(number), "D"(rdi) : "rcx", "r11", "memory"); // NOLINT
    return result;
}

template <typename Ret = ssize, typename T, typename U>
inline Ret invoke(Number number, const T &rdi, const U &rsi) {
    Ret result;                                                                                        // NOLINT
    asm volatile("syscall" : "=a"(result) : "a"(number), "D"(rdi), "S"(rsi) : "rcx", "r11", "memory"); // NOLINT
    return result;
}

template <typename Ret = ssize, typename T, typename U, typename V>
inline Ret invoke(Number number, const T &rdi, const U &rsi, const V &rdx) {
    Ret result; // NOLINT
    asm volatile("syscall"
                 : "=a"(result)
                 : "a"(number), "D"(rdi), "S"(rsi), "d"(rdx)
                 : "rcx", "r11", "memory"); // NOLINT
    return result;
}

} // namespace Syscall
