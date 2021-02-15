#pragma once

#include <ustd/Optional.hh>
#include <ustd/Types.hh>

struct BootInfo;

class MemoryManager {
    BootInfo *const m_boot_info;
    usize *m_frame_bitset;
    usize m_total_frames{0};

    Optional<uintptr> find_first_fit_region(usize size);

    usize &frame_bitset_bucket(usize index);
    void clear_frame(usize index);
    void set_frame(usize index);
    bool is_frame_set(usize index);

public:
    explicit MemoryManager(BootInfo *boot_info);

    // Allocate contiguous, eternal, page-aligned physical memory.
    void *alloc_phys(usize size);
};
