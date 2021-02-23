#pragma once

#include <kernel/cpu/Paging.hh>
#include <ustd/Types.hh>

class VirtSpace {
    Pml4 *const m_pml4;

public:
    VirtSpace();

    void map_4KiB(uintptr virt, uintptr phys, PageFlags flags = static_cast<PageFlags>(0));
    void map_1GiB(uintptr virt, uintptr phys, PageFlags flags = static_cast<PageFlags>(0));
    void switch_to();
};
