#pragma once

#include <kernel/cpu/Paging.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>

constexpr uintptr k_user_binary_base = 700_GiB;
constexpr uintptr k_user_stack_base = 600_GiB;
constexpr usize k_user_stack_page_count = 8;

constexpr uintptr k_user_stack_head = k_user_stack_base + k_user_stack_page_count * 4_KiB;

class MemoryManager;

class VirtSpace {
    friend MemoryManager;

private:
    Pml4 *m_pml4;

    VirtSpace();

public:
    static VirtSpace *create_user(void *binary, usize binary_size);

    VirtSpace(const VirtSpace &) = delete;
    VirtSpace(VirtSpace &&) = default;
    ~VirtSpace();

    VirtSpace &operator=(const VirtSpace &) = delete;
    VirtSpace &operator=(VirtSpace &&) = delete;

    void map_4KiB(uintptr virt, uintptr phys, PageFlags flags = static_cast<PageFlags>(0));
    void map_1GiB(uintptr virt, uintptr phys, PageFlags flags = static_cast<PageFlags>(0));
    void switch_to();

    VirtSpace *clone() const;
};
