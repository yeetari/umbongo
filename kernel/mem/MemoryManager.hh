#pragma once

#include <ustd/Types.hh>

struct BootInfo;
class VirtSpace;

struct MemoryManager {
    static void initialise(BootInfo *boot_info);
    static void reclaim(BootInfo *boot_info);
    static void switch_space(VirtSpace &virt_space);

    static uintptr alloc_frame();
    static void free_frame(uintptr frame);
    static bool is_frame_free(uintptr frame);

    static void *alloc_contiguous(usize size);
    static void free_contiguous(void *ptr, usize size);

    static VirtSpace *current_space();
    static VirtSpace *kernel_space();
};
