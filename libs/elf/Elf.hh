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

enum class SegmentType : uint32 {
    Load = 1,
};

enum class SegmentFlags : uint32 {
    Executable = 1u << 0u,
    Writable = 1u << 1u,
    Readable = 1u << 2u,
};

struct ProgramHeader {
    SegmentType type;
    SegmentFlags flags;
    ptrdiff offset;
    uintptr vaddr;
    uintptr paddr;
    uint64 filesz;
    uint64 memsz;
    uint64 align;
};

inline constexpr SegmentFlags operator&(SegmentFlags a, SegmentFlags b) {
    return static_cast<SegmentFlags>(static_cast<uint32>(a) & static_cast<uint32>(b));
}

} // namespace elf
