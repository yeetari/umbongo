#pragma once

#include <ustd/Types.hh>

namespace kernel {

enum class ThreadPriority : uint32_t {
    Idle = 0,
    Normal = 1,
};

} // namespace kernel
