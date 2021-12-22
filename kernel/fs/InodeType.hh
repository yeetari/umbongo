#pragma once

namespace kernel {

enum class InodeType {
    AnonymousFile,
    Directory,
    RegularFile,
};

} // namespace kernel
