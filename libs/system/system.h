#pragma once

#include <kernel/api/types.h> // IWYU pragma: export
// IWYU pragma: no_include "kernel/api/types.h"

typedef enum ub_syscall {
#define S(name, ...) UB_SYS_##name,
#include <kernel/api/syscalls.in>
#undef S
} ub_syscall_t;
