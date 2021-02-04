#pragma once

#include <ustd/Types.hh>

enum class MemoryType {
    Available,
    Reclaimable,
    Reserved,
};

struct MemoryMapEntry {
    MemoryType type;
    uintptr base;
    uint64 page_count;
};

struct BootInfo {
    // Framebuffer info.
    uint32 width;
    uint32 height;
    uint32 pixels_per_scan_line;
    uintptr framebuffer_base;

    // Memory map info.
    MemoryMapEntry *map;
    usize map_entry_count;
};
