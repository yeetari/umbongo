#include <kernel/mem/VirtSpace.hh>

#include <kernel/cpu/Paging.hh>
#include <kernel/cpu/Processor.hh>
#include <ustd/Assert.hh>
#include <ustd/Types.hh>

VirtSpace VirtSpace::create_user(void *binary, usize binary_size) {
    VirtSpace virt_space;
    for (usize offset = 0; offset <= round_up(binary_size, 4_KiB); offset += 4_KiB) {
        virt_space.map_4KiB(k_user_binary_base + offset, reinterpret_cast<uintptr>(binary) + offset, PageFlags::User);
    }
    auto *stack = new uint8[k_user_stack_page_count * 4_KiB];
    for (usize offset = 0; offset < k_user_stack_page_count * 4_KiB; offset += 4_KiB) {
        virt_space.map_4KiB(k_user_stack_base + offset, reinterpret_cast<uintptr>(stack) + offset,
                            PageFlags::Writable | PageFlags::User | PageFlags::NoExecute);
    }
    return virt_space;
}

VirtSpace::VirtSpace() : m_pml4(new Pml4) {
    // Identity map physical memory up to 512GiB. Using 1GiB pages means this only takes 4KiB of page structures to do.
    // TODO: Mark these pages as global.
    for (usize i = 0; i < 512; i++) {
        map_1GiB(i * 1_GiB, i * 1_GiB, PageFlags::Writable);
    }
}

void VirtSpace::map_4KiB(uintptr virt, uintptr phys, PageFlags flags) {
    ASSERT_PEDANTIC(virt % 4_KiB == 0);
    ASSERT_PEDANTIC(phys % 4_KiB == 0);
    const usize pml4_index = (virt >> 39ul) & 0x1fful;
    const usize pdpt_index = (virt >> 30ul) & 0x1fful;
    const usize pd_index = (virt >> 21ul) & 0x1fful;
    const usize pt_index = (virt >> 12ul) & 0x1fful;
    auto *pdpt = m_pml4->ensure(pml4_index);
    auto *pd = pdpt->ensure(pdpt_index);
    auto *pt = pd->ensure(pd_index);
    pt->set(pt_index, phys, flags);
}

void VirtSpace::map_1GiB(uintptr virt, uintptr phys, PageFlags flags) {
    ASSERT_PEDANTIC(virt % 1_GiB == 0);
    ASSERT_PEDANTIC(phys % 1_GiB == 0);
    const usize pml4_index = (virt >> 39ul) & 0x1fful;
    const usize pdpt_index = (virt >> 30ul) & 0x1fful;
    auto *pdpt = m_pml4->ensure(pml4_index);
    pdpt->set(pdpt_index, phys, flags | PageFlags::Large);
}

void VirtSpace::switch_to() {
    Processor::write_cr3(m_pml4);
}
