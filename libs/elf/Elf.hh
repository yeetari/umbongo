#pragma once

#include <ustd/Array.hh>
#include <ustd/Types.hh>

namespace elf {

struct Header {
    ustd::Array<uint8_t, 4> magic;
    ustd::Array<uint8_t, 12> ident;
    uint16_t type;
    uint16_t machine;
    uint32_t version;
    uintptr_t entry;
    size_t ph_off;
    size_t sh_off;
    uint32_t flags;
    uint16_t header_size;
    uint16_t ph_size;
    uint16_t ph_count;
};

enum class SegmentType : uint32_t {
    Load = 1,
    Dynamic = 2,
    Interp = 3,
};

enum class SegmentFlags : uint32_t {
    Executable = 1u << 0u,
    Writable = 1u << 1u,
    Readable = 1u << 2u,
};

struct ProgramHeader {
    SegmentType type;
    SegmentFlags flags;
    size_t offset;
    uintptr_t vaddr;
    uintptr_t paddr;
    uint64_t filesz;
    uint64_t memsz;
    uint64_t align;
};

enum class DynamicType : int64_t {
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
    uint64_t value;
};

enum class RelocationType : uint32_t {
    None = 0,
    Relative = 8,
};

struct Rela {
    size_t offset;
    uint64_t info;
    ssize_t addend;
};

inline constexpr SegmentFlags operator&(SegmentFlags a, SegmentFlags b) {
    return static_cast<SegmentFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

} // namespace elf
