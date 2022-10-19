#pragma once

enum ub_syscall_t {
#define S(name, ...) SYS_##name,
#include <kernel/api/Syscalls.in>
#undef S
};
