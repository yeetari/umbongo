#pragma once

#include <ustd/Types.hh>

struct BootInfo;

namespace kernel {

class VirtSpace;

struct MemoryManager {
    static void initialise(BootInfo *boot_info);
    static void reclaim(BootInfo *boot_info);
    static void switch_space(VirtSpace &virt_space);

    static uintptr_t alloc_frame();
    static void free_frame(uintptr_t frame);
    static bool is_frame_free(uintptr_t frame);

    static void *alloc_contiguous(size_t size);
    static void free_contiguous(void *ptr, size_t size);

    static VirtSpace *current_space();
    static VirtSpace *kernel_space();
};

} // namespace kernel
