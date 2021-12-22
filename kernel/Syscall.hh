#pragma once

#include <kernel/SysError.hh>
#include <ustd/Result.hh>
#include <ustd/Types.hh>

namespace Syscall {

#define ENUMERATE_SYSCALLS(S)                                                                                          \
    S(accept)                                                                                                          \
    S(allocate_region)                                                                                                 \
    S(chdir)                                                                                                           \
    S(close)                                                                                                           \
    S(connect)                                                                                                         \
    S(create_pipe)                                                                                                     \
    S(create_process)                                                                                                  \
    S(create_server_socket)                                                                                            \
    S(dup_fd)                                                                                                          \
    S(exit)                                                                                                            \
    S(getcwd)                                                                                                          \
    S(getpid)                                                                                                          \
    S(gettime)                                                                                                         \
    S(ioctl)                                                                                                           \
    S(mkdir)                                                                                                           \
    S(mmap)                                                                                                            \
    S(mount)                                                                                                           \
    S(open)                                                                                                            \
    S(poll)                                                                                                            \
    S(putchar)                                                                                                         \
    S(read)                                                                                                            \
    S(read_directory)                                                                                                  \
    S(seek)                                                                                                            \
    S(size)                                                                                                            \
    S(virt_to_phys)                                                                                                    \
    S(wait_pid)                                                                                                        \
    S(write)

enum Number {
#define ENUMERATE_SYSCALL(s) s,
    ENUMERATE_SYSCALLS(ENUMERATE_SYSCALL)
#undef ENUMERATE_SYSCALL
        __Count__,
};

template <typename R = usize>
inline ustd::Result<R, SysError> invoke(Number number) {
    ssize result; // NOLINT
    asm volatile("syscall" : "=a"(result) : "a"(number) : "rcx", "r11", "memory");
    if (result < 0) {
        return static_cast<SysError>(result);
    }
    return R(result);
}

template <typename R = usize, typename T>
inline ustd::Result<R, SysError> invoke(Number number, const T &rdi) {
    ssize result;                                                                            // NOLINT
    asm volatile("syscall" : "=a"(result) : "a"(number), "D"(rdi) : "rcx", "r11", "memory"); // NOLINT
    if (result < 0) {
        return static_cast<SysError>(result);
    }
    return R(result);
}

template <typename R = usize, typename T, typename U>
inline ustd::Result<R, SysError> invoke(Number number, const T &rdi, const U &rsi) {
    ssize result;                                                                                      // NOLINT
    asm volatile("syscall" : "=a"(result) : "a"(number), "D"(rdi), "S"(rsi) : "rcx", "r11", "memory"); // NOLINT
    if (result < 0) {
        return static_cast<SysError>(result);
    }
    return R(result);
}

template <typename R = usize, typename T, typename U, typename V>
inline ustd::Result<R, SysError> invoke(Number number, const T &rdi, const U &rsi, const V &rdx) {
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

} // namespace Syscall
