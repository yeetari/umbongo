#pragma once

#include <ustd/Types.hh>

enum class SysError : ssize {
    BadFd = -1,
    NonExistent = -2,
    BrokenHandle = -3,
    Invalid = -4,
    NoExec = -5,
    NotDirectory = -6,
    AlreadyExists = -7,
    Busy = -8,
};
