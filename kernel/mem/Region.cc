#include <kernel/mem/Region.hh>

#include <kernel/cpu/InterruptDisabler.hh>
#include <kernel/cpu/Paging.hh>
#include <kernel/mem/MemoryManager.hh>
#include <kernel/mem/PhysicalPage.hh>
#include <kernel/mem/VirtSpace.hh>
#include <ustd/Optional.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

namespace {

PageFlags page_flags(RegionAccess access) {
    auto flags = static_cast<PageFlags>(0);
    if ((access & RegionAccess::Writable) == RegionAccess::Writable) {
        flags |= PageFlags::Writable;
    }
    if ((access & RegionAccess::Executable) != RegionAccess::Executable) {
        flags |= PageFlags::NoExecute;
    }
    if ((access & RegionAccess::UserAccessible) == RegionAccess::UserAccessible) {
        flags |= PageFlags::User;
    }
    return flags;
}

} // namespace

Region::Region(uintptr base, usize size, RegionAccess access, bool free, Optional<uintptr> phys_base)
    : m_base(base), m_size(size), m_access(access), m_free(free) {
    if (free) {
        return;
    }
    if (!phys_base) {
        do {
            const usize map_size = size >= 1_GiB ? 1_GiB : size >= 2_MiB ? 2_MiB : 4_KiB;
            const auto page_size = map_size == 1_GiB   ? PhysicalPageSize::Huge
                                   : map_size == 2_MiB ? PhysicalPageSize::Large
                                                       : PhysicalPageSize::Normal;
            m_physical_pages.push(PhysicalPage::allocate(page_size));
            size -= map_size;
        } while (size != 0);
        return;
    }
    // TODO: Use 2 MiB pages.
    uintptr phys = *phys_base;
    do {
        const usize map_size = size >= 1_GiB ? 1_GiB : 4_KiB;
        const auto page_size = map_size == 1_GiB ? PhysicalPageSize::Huge : PhysicalPageSize::Normal;
        m_physical_pages.push(PhysicalPage::create(phys, page_size));
        phys += map_size;
        size -= map_size;
    } while (size != 0);
}

void Region::map(VirtSpace *virt_space) const {
    const auto flags = page_flags(m_access);
    for (uintptr virt = m_base; const auto &physical_page : m_physical_pages) {
        switch (physical_page->size()) {
        case PhysicalPageSize::Normal:
            virt_space->map_4KiB(virt, physical_page->phys(), flags);
            virt += 4_KiB;
            break;
        case PhysicalPageSize::Large:
            virt_space->map_2MiB(virt, physical_page->phys(), flags);
            virt += 2_MiB;
            break;
        case PhysicalPageSize::Huge:
            virt_space->map_1GiB(virt, physical_page->phys(), flags);
            virt += 1_GiB;
            break;
        }
    }

    InterruptDisabler disabler;
    if (MemoryManager::current_space() == virt_space) {
        for (uintptr virt = m_base; virt < m_base + m_size; virt += 4_KiB) {
            asm volatile("invlpg (%0)" : : "r"(virt) : "memory");
        }
    }
}
