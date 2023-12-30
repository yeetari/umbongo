#pragma once

#include <ustd/types.hh>

struct BootInfo;

namespace kernel {

class VirtSpace;

struct MemoryManager {
    static void initialise(BootInfo *boot_info);
    static void reclaim(BootInfo *boot_info);

    static uintptr_t alloc_frame();
    static void free_frame(uintptr_t frame);
    static bool is_frame_free(uintptr_t frame);

    static void *alloc_contiguous(size_t size);
    static void free_contiguous(void *ptr, size_t size);

    static VirtSpace *kernel_space();
};

} // namespace kernel
