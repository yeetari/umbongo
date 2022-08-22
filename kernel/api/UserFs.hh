#pragma once

#include <kernel/SysError.hh>
#include <ustd/Array.hh>

namespace kernel {

enum class UserFsOpcode {
    Child,
    Create, // str(name), InodeType(type) -> InodeId
    Lookup,
    Read,
    Remove, // str(name)
    Size,
    Truncate,
    Write, // offset
    Name,  // -> str(name)
};

struct UserFsRequest {
    usize data_size;
    union {
        usize index;  // For Child
        usize offset; // For Read
    };
    UserFsOpcode opcode;
};

struct UserFsResponse {
    usize data_size;   // For Read
    usize inode_index; // For Child, Lookup
    usize size;        // For Size
    SysError error;
};

} // namespace kernel
