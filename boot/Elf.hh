#pragma once

#include <ustd/Types.hh>

struct [[gnu::packed]] ElfHeader {
    uint8 magic[4];
    uint8 ident[12];
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

enum class ElfProgramHeaderType : uint32 {
    Load = 1,
};

struct [[gnu::packed]] ElfProgramHeader {
    ElfProgramHeaderType type;
    uint32 flags;
    ptrdiff offset;
    uintptr vaddr;
    uintptr paddr;
    uint64 filesz;
    uint64 memsz;
    uint64 align;
};
