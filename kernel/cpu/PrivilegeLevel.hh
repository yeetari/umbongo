#pragma once

#include <ustd/Types.hh>

enum class PrivilegeLevel : uint8 {
    Kernel = 0,
    User = 3,
};
