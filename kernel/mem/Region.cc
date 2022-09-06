#include <kernel/mem/Region.hh>

#include <kernel/cpu/Paging.hh>
#include <kernel/mem/MemoryManager.hh>
#include <kernel/mem/PhysicalPage.hh>
#include <kernel/mem/VirtSpace.hh>
#include <ustd/Optional.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

namespace kernel {
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
    if ((access & RegionAccess::Uncacheable) == RegionAccess::Uncacheable) {
        flags |= PageFlags::CacheDisable;
    }
    if ((access & RegionAccess::Global) == RegionAccess::Global) {
        flags |= PageFlags::Global;
    }
    return flags;
}

} // namespace

Region::Region(uintptr_t base, size_t size, RegionAccess access, bool free, ustd::Optional<uintptr_t> phys_base)
    : m_base(base), m_size(size), m_access(access), m_free(free) {
    if (free) {
        return;
    }
    if (!phys_base) {
        do {
            const size_t map_size = size >= 1_GiB ? 1_GiB : size >= 2_MiB ? 2_MiB : 4_KiB;
            const auto page_size = map_size == 1_GiB   ? PhysicalPageSize::Huge
                                   : map_size == 2_MiB ? PhysicalPageSize::Large
                                                       : PhysicalPageSize::Normal;
            m_physical_pages.push(PhysicalPage::allocate(page_size));
            size -= map_size;
        } while (size != 0);
        return;
    }
    uintptr_t phys = *phys_base;
    do {
        const size_t map_size = size >= 1_GiB ? 1_GiB : size >= 2_MiB ? 2_MiB : 4_KiB;
        const auto page_size = map_size == 1_GiB   ? PhysicalPageSize::Huge
                               : map_size == 2_MiB ? PhysicalPageSize::Large
                                                   : PhysicalPageSize::Normal;
        m_physical_pages.push(PhysicalPage::create(phys, page_size));
        phys += map_size;
        size -= map_size;
    } while (size != 0);
}

void Region::map(VirtSpace *virt_space) const {
    const auto flags = page_flags(m_access);
    for (uintptr_t virt = m_base; const auto &physical_page : m_physical_pages) {
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

    if (MemoryManager::current_space() == virt_space) {
        for (uintptr_t virt = m_base; virt < m_base + m_size; virt += 4_KiB) {
            asm volatile("invlpg (%0)" : : "r"(virt) : "memory");
        }
    }
}

} // namespace kernel
