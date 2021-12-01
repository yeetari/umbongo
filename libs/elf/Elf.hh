#pragma once

#include <ustd/Array.hh>
#include <ustd/Types.hh>

namespace elf {

struct Header {
    ustd::Array<uint8, 4> magic;
    ustd::Array<uint8, 12> ident;
    uint16 type;
    uint16 machine;
    uint32 version;
    uintptr entry;
    usize ph_off;
    usize sh_off;
    uint32 flags;
    uint16 header_size;
    uint16 ph_size;
    uint16 ph_count;
};

enum class SegmentType : uint32 {
    Load = 1,
    Dynamic = 2,
    Interp = 3,
};

enum class SegmentFlags : uint32 {
    Executable = 1u << 0u,
    Writable = 1u << 1u,
    Readable = 1u << 2u,
};

struct ProgramHeader {
    SegmentType type;
    SegmentFlags flags;
    usize offset;
    uintptr vaddr;
    uintptr paddr;
    uint64 filesz;
    uint64 memsz;
    uint64 align;
};

enum class DynamicType : int64 {
    Null = 0,
    Hash = 4,
    StrTab = 5,
    SymTab = 6,
    Rela = 7,
    RelaSz = 8,
    RelaEnt = 9,
    StrSz = 10,
    SymEnt = 11,
    Debug = 21,
    Flags = 30,
    GnuHash = 0x6ffffef5,
    RelaCount = 0x6ffffff9,
    Flags1 = 0x6ffffffb,
};

struct DynamicEntry {
    DynamicType type;
    uint64 value;
};

enum class RelocationType : uint32 {
    None = 0,
    Relative = 8,
};

struct Rela {
    usize offset;
    uint64 info;
    ssize addend;
};

inline constexpr SegmentFlags operator&(SegmentFlags a, SegmentFlags b) {
    return static_cast<SegmentFlags>(static_cast<uint32>(a) & static_cast<uint32>(b));
}

} // namespace elf
