#pragma once

#include <ustd/Array.hh>
#include <ustd/Types.hh>

struct FdPair {
    uint32 parent;
    uint32 child;
};

struct FramebufferInfo {
    usize size;
    uint32 width;
    uint32 height;
};

struct PciDeviceInfo {
    ustd::Array<uintptr, 6> bars;
    uint16 vendor_id;
    uint16 device_id;
};

enum class PollEvents : uint16 {
    Read = 1u << 0u,
    Write = 1u << 1u,
    Accept = Read,
};

struct PollFd {
    uint32 fd;
    PollEvents events;
    PollEvents revents;
};

enum class IoctlRequest : usize {
    FramebufferGetInfo,
};

enum class MemoryProt : usize {
    Write = 1u << 0u,
    Exec = 1u << 1u,
    Uncacheable = 1u << 2u,
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

inline constexpr PollEvents operator&(PollEvents a, PollEvents b) {
    return static_cast<PollEvents>(static_cast<usize>(a) & static_cast<usize>(b));
}

inline constexpr PollEvents operator|(PollEvents a, PollEvents b) {
    return static_cast<PollEvents>(static_cast<usize>(a) | static_cast<usize>(b));
}

inline constexpr PollEvents operator|=(PollEvents &a, PollEvents b) {
    return a = (a | b);
}

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
