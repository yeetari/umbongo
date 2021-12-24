#pragma once

namespace Syscall {

#define ENUMERATE_SYSCALLS(S)                                                                                          \
    S(accept)                                                                                                          \
    S(allocate_region)                                                                                                 \
    S(bind)                                                                                                            \
    S(chdir)                                                                                                           \
    S(close)                                                                                                           \
    S(connect)                                                                                                         \
    S(create_pipe)                                                                                                     \
    S(create_process)                                                                                                  \
    S(create_server_socket)                                                                                            \
    S(debug_line)                                                                                                      \
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

} // namespace Syscall
