#include <kernel/mem/Region.hh>

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
    // TODO: Use 2 MiB pages.
    if (!phys_base) {
        // TODO: Allocate huge pages here too.
        for (usize i = 0; i < size / 4_KiB; i++) {
            m_physical_pages.push(PhysicalPage::allocate());
        }
        return;
    }
    uintptr phys = *phys_base;
    uintptr virt = base;
    usize remaining_size = size;
    do {
        const usize map_size = remaining_size >= 1_GiB ? 1_GiB : 4_KiB;
        const auto page_size = map_size == 1_GiB ? PhysicalPageSize::Huge : PhysicalPageSize::Normal;
        m_physical_pages.push(PhysicalPage::create(phys, page_size));
        phys += map_size;
        virt += map_size;
        remaining_size -= map_size;
    } while (remaining_size != 0);
}

void Region::map(VirtSpace *virt_space) const {
    const auto flags = page_flags(m_access);
    for (uintptr virt = m_base; const auto &physical_page : m_physical_pages) {
        switch (physical_page->size()) {
        case PhysicalPageSize::Normal:
            virt_space->map_4KiB(virt, physical_page->phys(), flags);
            virt += 4_KiB;
            break;
        case PhysicalPageSize::Huge:
            virt_space->map_1GiB(virt, physical_page->phys(), flags);
            virt += 1_GiB;
            break;
        }
    }

    if (MemoryManager::current_space() == virt_space) {
        for (uintptr virt = m_base; virt < m_base + m_size; virt += 4_KiB) {
            asm volatile("invlpg (%0)" : : "r"(virt) : "memory");
        }
    }
}
