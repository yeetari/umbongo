#pragma once

#include <kernel/api/Constants.h> // IWYU pragma: export
#include <kernel/api/Types.h> // IWYU pragma: export

enum ub_error_t : __INTPTR_TYPE__ {
#define E(name, _, value) UB_ERROR_##name = (value),
#include <kernel/api/Errors.in>
#undef E
};
