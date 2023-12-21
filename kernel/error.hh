#pragma once

#include <ustd/types.hh>

namespace kernel {

enum class Error : ssize_t {
#define E(_, name, value) name = (value),
#include <kernel/api/errors.in>
#undef E
};

} // namespace kernel
