#pragma once

#include <ustd/Types.hh>

struct BootInfo {
    // Framebuffer info.
    uint32 width;
    uint32 height;
    uint32 pixels_per_scan_line;
    uintptr framebuffer_base;
};
