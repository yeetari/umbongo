#include <kernel/mem/VirtSpace.hh>

#include <kernel/ScopedLock.hh>
#include <kernel/SpinLock.hh>
#include <kernel/cpu/Paging.hh>
#include <kernel/mem/Region.hh>
#include <ustd/Assert.hh>
#include <ustd/Numeric.hh>
#include <ustd/Optional.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>
#include <ustd/Utility.hh>
#include <ustd/Vector.hh>

namespace kernel {

VirtSpace::VirtSpace() : m_pml4(new Pml4) {
    m_regions.push(ustd::make_unique<Region>(0ul, 256_TiB, static_cast<RegionAccess>(0), true, 0ul));
}

VirtSpace::VirtSpace(const ustd::Vector<ustd::UniquePtr<Region>> &regions) : m_pml4(new Pml4) {
    m_regions.ensure_capacity(regions.size());
    for (const auto &region : regions) {
        m_regions.push(ustd::make_unique<Region>(*region));
    }
    for (auto &region : m_regions) {
        region->map(this);
    }
}

void VirtSpace::map_4KiB(uintptr_t virt, uintptr_t phys, PageFlags flags) {
    ASSERT_PEDANTIC(virt % 4_KiB == 0);
    ASSERT_PEDANTIC(phys % 4_KiB == 0);
    const size_t pml4_index = (virt >> 39ul) & 0x1fful;
    const size_t pdpt_index = (virt >> 30ul) & 0x1fful;
    const size_t pd_index = (virt >> 21ul) & 0x1fful;
    const size_t pt_index = (virt >> 12ul) & 0x1fful;
    auto *pdpt = m_pml4->ensure(pml4_index);
    auto *pd = pdpt->ensure(pdpt_index);
    auto *pt = pd->ensure(pd_index);
    pt->set(pt_index, phys, flags);
}

void VirtSpace::map_2MiB(uintptr_t virt, uintptr_t phys, PageFlags flags) {
    ASSERT_PEDANTIC(virt % 2_MiB == 0);
    ASSERT_PEDANTIC(phys % 2_MiB == 0);
    const size_t pml4_index = (virt >> 39ul) & 0x1fful;
    const size_t pdpt_index = (virt >> 30ul) & 0x1fful;
    const size_t pd_index = (virt >> 21ul) & 0x1fful;
    auto *pdpt = m_pml4->ensure(pml4_index);
    auto *pd = pdpt->ensure(pdpt_index);
    pd->set(pd_index, phys, flags | PageFlags::Large);
}

void VirtSpace::map_1GiB(uintptr_t virt, uintptr_t phys, PageFlags flags) {
    ASSERT_PEDANTIC(virt % 1_GiB == 0);
    ASSERT_PEDANTIC(phys % 1_GiB == 0);
    const size_t pml4_index = (virt >> 39ul) & 0x1fful;
    const size_t pdpt_index = (virt >> 30ul) & 0x1fful;
    auto *pdpt = m_pml4->ensure(pml4_index);
    pdpt->set(pdpt_index, phys, flags | PageFlags::Large);
}

Region &VirtSpace::allocate_region(size_t size, RegionAccess access, ustd::Optional<uintptr_t> phys_base) {
    // Find first fit region to split.
    ScopedLock locker(m_lock);
    size = ustd::align_up(size, 4_KiB);
    const size_t alignment = size >= 1_GiB ? 1_GiB : size >= 2_MiB ? 2_MiB : 4_KiB;
    for (auto &region : m_regions) {
        if (!region->free()) {
            continue;
        }
        uintptr_t base = ustd::align_up(region->base(), alignment);
        size_t padding = base - region->base();
        if (region->size() >= size + padding) {
            ASSERT(base % alignment == 0);
            size_t original_size = region->size();
            region->set_size(padding);
            auto &new_region =
                *m_regions.emplace(ustd::make_unique<Region>(base, size, access, false, ustd::move(phys_base)));
            m_regions.emplace(ustd::make_unique<Region>(base + size, original_size - (size + padding),
                                                        static_cast<RegionAccess>(0), true, 0ul));
            new_region.map(this);
            return new_region;
        }
    }
    ENSURE_NOT_REACHED("Failed to allocate region");
}

Region &VirtSpace::create_region(uintptr_t base, size_t size, RegionAccess access,
                                 ustd::Optional<uintptr_t> phys_base) {
    ScopedLock locker(m_lock);
    size = ustd::align_up(size, 4_KiB);
    for (auto &region : m_regions) {
        if (!region->free()) {
            continue;
        }
        if (base >= region->base() && base + size < region->base() + region->size()) {
            size_t preceding_size = base - region->base();
            size_t succeeding_size = (region->base() + region->size()) - (base + size);
            region = ustd::make_unique<Region>(region->base(), preceding_size, static_cast<RegionAccess>(0), true, 0ul);
            auto &new_region =
                *m_regions.emplace(ustd::make_unique<Region>(base, size, access, false, ustd::move(phys_base)));
            m_regions.emplace(
                ustd::make_unique<Region>(base + size, succeeding_size, static_cast<RegionAccess>(0), true, 0ul));
            new_region.map(this);
            return new_region;
        }
    }
    ENSURE_NOT_REACHED("Failed to allocate fixed region");
}

ustd::SharedPtr<VirtSpace> VirtSpace::clone() const {
    ScopedLock locker(m_lock);
    return ustd::SharedPtr<VirtSpace>(new VirtSpace(m_regions));
}

} // namespace kernel
