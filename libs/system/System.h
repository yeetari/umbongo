#pragma once

#include <kernel/api/Types.h> // IWYU pragma: export
// IWYU pragma: no_include "kernel/api/Types.h"

typedef enum ub_syscall {
#define S(name, ...) UB_SYS_##name,
#include <kernel/api/Syscalls.in>
#undef S
} ub_syscall_t;
