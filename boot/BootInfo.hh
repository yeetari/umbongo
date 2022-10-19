#pragma once

#include <ustd/Types.hh>

enum class MemoryType {
    Available,
    Reclaimable,
    Reserved,
};

struct MemoryMapEntry {
    MemoryType type;
    uintptr_t base;
    uint64_t page_count;
};

struct RamFsEntry {
    size_t data_size{0};
    uint16_t name_length{0};
    bool is_directory{false};
    RamFsEntry *next{nullptr};
};

struct BootInfo {
    // Dmesg ring buffer memory.
    void *dmesg_area;

    // ACPI RSDP.
    void *rsdp;

    // Framebuffer info.
    uint32_t width;
    uint32_t height;
    uint32_t pixels_per_scan_line;
    uintptr_t framebuffer_base;

    // Memory map info.
    MemoryMapEntry *map;
    size_t map_entry_count;

    // Initial files needed to boot.
    RamFsEntry *initramfs;
};
