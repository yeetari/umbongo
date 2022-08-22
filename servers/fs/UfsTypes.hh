#pragma once

#include <ustd/Types.hh>

namespace ufs {

constexpr uint32 k_superblock_magic = 0x21534655;

constexpr uint32 blocks_per_group(uint16 block_size) {
    // How many bits in a full block (for the block bitmap).
    return block_size * 8;
}

using BlockIndex = uint32;
using GroupIndex = uint32;

struct Superblock {
    uint32 magic;
    uint32 block_count;
    // TODO: Store as log2(block_size) to avoid divides in hot FS code.
    uint16 block_size;
};

struct GroupHeader {
    BlockIndex block_bitmap;
    BlockIndex block_table;
};

enum class InodeType : uint8 {
    Directory = 0,
    RegularFile = 1,
};

struct InodeHeader {
    uint64 size;
    InodeType type;
};

// TODO: Make 256 bytes to avoid divide cost in traverse_directory?
struct DirectoryEntry {
    BlockIndex inode;
    uint8 name_length;
    char name[255]; // NOLINT
};

} // namespace ufs
