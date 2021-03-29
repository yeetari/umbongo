#pragma once

#include <ustd/Optional.hh>
#include <ustd/Types.hh>

struct BootInfo;

class MemoryManager {
    BootInfo *const m_boot_info{nullptr};
    usize *m_frame_bitset{nullptr};
    usize m_total_frames{0};

    Optional<uintptr> find_first_fit_region(usize size);
    usize &frame_bitset_bucket(usize index);
    void clear_frame(usize index);
    void set_frame(usize index);
    bool is_frame_set(usize index);

public:
    static void initialise(BootInfo *boot_info);

    constexpr MemoryManager() = default;
    explicit MemoryManager(BootInfo *boot_info);

    // Allocate contiguous, page-aligned physical memory.
    void *alloc_phys(usize size);
    void free_phys(void *ptr, usize size);
};
