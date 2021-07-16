#include <kernel/mem/VirtSpace.hh>

#include <kernel/cpu/Paging.hh>
#include <kernel/cpu/Processor.hh>
#include <kernel/mem/MemoryManager.hh>
#include <ustd/Assert.hh>
#include <ustd/Memory.hh>
#include <ustd/Types.hh>

VirtSpace *VirtSpace::create_user() {
    auto *virt_space = MemoryManager::kernel_space()->clone();
    auto *stack = new (ustd::align_val_t(4_KiB)) uint8[k_user_stack_page_count * 4_KiB];
    for (usize offset = 0; offset < k_user_stack_page_count * 4_KiB; offset += 4_KiB) {
        virt_space->map_4KiB(k_user_stack_base + offset, reinterpret_cast<uintptr>(stack) + offset,
                             PageFlags::Writable | PageFlags::User | PageFlags::NoExecute);
    }
    return virt_space;
}

VirtSpace::VirtSpace() : m_pml4(new Pml4) {}

VirtSpace::~VirtSpace() {
    delete m_pml4;
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

VirtSpace *VirtSpace::clone() const {
    // Deep clone this virt space. Note that pdpt_index means the index of the PD in the PDPT, rather than the index of
    // the PDPT in the PML4.
    auto *new_space = new VirtSpace;
    for (usize pml4_index = 0; pml4_index < 512; pml4_index++) {
        const auto &pdpt = m_pml4->entries()[pml4_index];
        if (pdpt.empty()) {
            continue;
        }
        for (usize pdpt_index = 0; pdpt_index < 512; pdpt_index++) {
            const auto &pd = pdpt.entry()->entries()[pdpt_index];
            if (pd.empty()) {
                continue;
            }
            if ((pd.flags() & PageFlags::Large) == PageFlags::Large) {
                const auto virt = (pml4_index * 512_GiB) + (pdpt_index * 1_GiB);
                const auto phys = reinterpret_cast<uintptr>(pd.entry());
                new_space->map_1GiB(virt, phys, pd.flags());
                continue;
            }
            for (usize pd_index = 0; pd_index < 512; pd_index++) {
                const auto &pt = pd.entry()->entries()[pd_index];
                if (pt.empty()) {
                    continue;
                }
                for (usize pt_index = 0; pt_index < 512; pt_index++) {
                    const auto &page = pt.entry()->entries()[pt_index];
                    if (page.empty()) {
                        continue;
                    }
                    const auto virt =
                        (pml4_index * 512_GiB) + (pdpt_index * 1_GiB) + (pd_index * 2_MiB) + (pt_index * 4_KiB);
                    const auto phys = reinterpret_cast<uintptr>(page.entry());
                    new_space->map_4KiB(virt, phys, page.flags());
                }
            }
        }
    }
    return new_space;
}
