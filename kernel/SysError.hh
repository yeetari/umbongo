#pragma once

#include <ustd/Types.hh>

namespace kernel {

enum class SysError : ssize_t {
    BadFd = -1,
    NonExistent = -2,
    BrokenHandle = -3,
    Invalid = -4,
    NoExec = -5,
    NotDirectory = -6,
    AlreadyExists = -7,
    Busy = -8,
};

} // namespace kernel
