#pragma once

#include <ustd/Types.hh>

namespace Syscall {

#define ENUMERATE_SYSCALLS(S) S(print)

enum Numbers {
#define ENUMERATE_SYSCALL(s) s,
    ENUMERATE_SYSCALLS(ENUMERATE_SYSCALL)
#undef ENUMERATE_SYSCALL
        __Count__,
};

} // namespace Syscall

uint64 sys_print();
