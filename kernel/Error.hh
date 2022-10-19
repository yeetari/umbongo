#pragma once

#include <ustd/Types.hh>

namespace kernel {

enum class Error : ssize_t {
#define E(_, name, value) name = (value),
#include <kernel/api/Errors.in>
#undef E
};

} // namespace kernel
