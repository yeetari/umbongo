#pragma once

#include <ustd/Types.hh>

enum class SysError : ssize {
    BadFd = -1,
    NonExistent = -2,
};
