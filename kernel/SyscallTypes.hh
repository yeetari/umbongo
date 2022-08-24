#pragma once

#include <ustd/Array.hh>
#include <ustd/Types.hh>

namespace kernel {

struct FdPair {
    uint32_t parent;
    uint32_t child;
};

struct FramebufferInfo {
    size_t size;
    uint32_t width;
    uint32_t height;
};

struct PciBar {
    uintptr_t address;
    size_t size;
};

struct PciDeviceInfo {
    ustd::Array<PciBar, 6> bars;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t clas;
    uint8_t subc;
    uint8_t prif;
};

enum class PollEvents : uint16_t {
    Read = 1u << 0u,
    Write = 1u << 1u,
    Accept = Read,
};

struct PollFd {
    uint32_t fd;
    PollEvents events;
    PollEvents revents;
};

enum class IoctlRequest : size_t {
    FramebufferGetInfo,
    PciEnableDevice,
    PciEnableInterrupts,
};

enum class MemoryProt : size_t {
    Write = 1u << 0u,
    Exec = 1u << 1u,
    Uncacheable = 1u << 2u,
};

enum class OpenMode : size_t {
    None = 0,
    Create = 1u << 0u,
    Truncate = 1u << 1u,
};

enum class SeekMode {
    Add,
    Set,
};

inline constexpr PollEvents operator&(PollEvents a, PollEvents b) {
    return static_cast<PollEvents>(static_cast<size_t>(a) & static_cast<size_t>(b));
}

inline constexpr PollEvents operator|(PollEvents a, PollEvents b) {
    return static_cast<PollEvents>(static_cast<size_t>(a) | static_cast<size_t>(b));
}

inline constexpr PollEvents operator|=(PollEvents &a, PollEvents b) {
    return a = (a | b);
}

inline constexpr MemoryProt operator&(MemoryProt a, MemoryProt b) {
    return static_cast<MemoryProt>(static_cast<size_t>(a) & static_cast<size_t>(b));
}

inline constexpr MemoryProt operator|(MemoryProt a, MemoryProt b) {
    return static_cast<MemoryProt>(static_cast<size_t>(a) | static_cast<size_t>(b));
}

inline constexpr OpenMode operator&(OpenMode a, OpenMode b) {
    return static_cast<OpenMode>(static_cast<size_t>(a) & static_cast<size_t>(b));
}

inline constexpr OpenMode operator|(OpenMode a, OpenMode b) {
    return static_cast<OpenMode>(static_cast<size_t>(a) | static_cast<size_t>(b));
}

inline constexpr OpenMode &operator|=(OpenMode &a, OpenMode b) {
    return a = (a | b);
}

} // namespace kernel
