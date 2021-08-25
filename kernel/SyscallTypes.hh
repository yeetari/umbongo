#pragma once

#include <ustd/Types.hh>

struct FdPair {
    uint32 parent;
    uint32 child;
};

struct FramebufferInfo {
    uint32 width;
    uint32 height;
};

struct TerminalSize {
    uint32 column_count;
    uint32 row_count;
};

enum class IoctlRequest : usize {
    FramebufferClear,
    FramebufferGetInfo,
    TerminalGetSize,
};

enum class MemoryProt : usize {
    Write = 1u << 0u,
    Exec = 1u << 1u,
};

enum class OpenMode : usize {
    None = 0,
    Create = 1u << 0u,
    Truncate = 1u << 1u,
};

enum class SeekMode {
    Add,
    Set,
};

inline constexpr MemoryProt operator&(MemoryProt a, MemoryProt b) {
    return static_cast<MemoryProt>(static_cast<usize>(a) & static_cast<usize>(b));
}

inline constexpr MemoryProt operator|(MemoryProt a, MemoryProt b) {
    return static_cast<MemoryProt>(static_cast<usize>(a) | static_cast<usize>(b));
}

inline constexpr OpenMode operator&(OpenMode a, OpenMode b) {
    return static_cast<OpenMode>(static_cast<usize>(a) & static_cast<usize>(b));
}

inline constexpr OpenMode operator|(OpenMode a, OpenMode b) {
    return static_cast<OpenMode>(static_cast<usize>(a) | static_cast<usize>(b));
}

inline constexpr OpenMode &operator|=(OpenMode &a, OpenMode b) {
    return a = (a | b);
}
