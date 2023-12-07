#pragma once

typedef enum ub_error {
#define E(name, _, value) UB_ERROR_##name = (value),
#include <kernel/api/Errors.in>
#undef E
} ub_error_t;
