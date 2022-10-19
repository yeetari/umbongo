#pragma once

namespace kernel {

enum class HandleKind {
    Channel,
    Endpoint,
    Region,
    VmObject,
};

} // namespace kernel
