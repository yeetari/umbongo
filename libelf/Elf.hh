#pragma once

#include <ustd/Array.hh>
#include <ustd/Types.hh>

namespace elf {

struct Header {
    Array<uint8, 4> magic;
    Array<uint8, 12> ident;
    uint16 type;
    uint16 machine;
    uint32 version;
    uintptr entry;
    ptrdiff ph_off;
    ptrdiff sh_off;
    uint32 flags;
    uint16 header_size;
    uint16 ph_size;
    uint16 ph_count;
};

enum class ProgramHeaderType : uint32 {
    Load = 1,
};

struct ProgramHeader {
    ProgramHeaderType type;
    uint32 flags;
    ptrdiff offset;
    uintptr vaddr;
    uintptr paddr;
    uint64 filesz;
    uint64 memsz;
    uint64 align;
};

} // namespace elf
