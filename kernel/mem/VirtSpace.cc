#include <kernel/mem/VirtSpace.hh>

#include <kernel/ScopedLock.hh> // IWYU pragma: keep
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

void VirtSpace::map_2MiB(uintptr virt, uintptr phys, PageFlags flags) {
    ASSERT_PEDANTIC(virt % 2_MiB == 0);
    ASSERT_PEDANTIC(phys % 2_MiB == 0);
    const usize pml4_index = (virt >> 39ul) & 0x1fful;
    const usize pdpt_index = (virt >> 30ul) & 0x1fful;
    const usize pd_index = (virt >> 21ul) & 0x1fful;
    auto *pdpt = m_pml4->ensure(pml4_index);
    auto *pd = pdpt->ensure(pdpt_index);
    pd->set(pd_index, phys, flags | PageFlags::Large);
}

void VirtSpace::map_1GiB(uintptr virt, uintptr phys, PageFlags flags) {
    ASSERT_PEDANTIC(virt % 1_GiB == 0);
    ASSERT_PEDANTIC(phys % 1_GiB == 0);
    const usize pml4_index = (virt >> 39ul) & 0x1fful;
    const usize pdpt_index = (virt >> 30ul) & 0x1fful;
    auto *pdpt = m_pml4->ensure(pml4_index);
    pdpt->set(pdpt_index, phys, flags | PageFlags::Large);
}

Region &VirtSpace::allocate_region(usize size, RegionAccess access, ustd::Optional<uintptr> phys_base) {
    // Find first fit region to split.
    ScopedLock locker(m_lock);
    size = ustd::align_up(size, 4_KiB);
    const usize alignment = size >= 1_GiB ? 1_GiB : size >= 2_MiB ? 2_MiB : 4_KiB;
    for (auto &region : m_regions) {
        if (!region->free()) {
            continue;
        }
        uintptr base = ustd::align_up(region->base(), alignment);
        usize padding = base - region->base();
        if (region->size() >= size + padding) {
            ASSERT(base % alignment == 0);
            usize original_size = region->size();
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

Region &VirtSpace::create_region(uintptr base, usize size, RegionAccess access, ustd::Optional<uintptr> phys_base) {
    ScopedLock locker(m_lock);
    size = ustd::align_up(size, 4_KiB);
    for (auto &region : m_regions) {
        if (!region->free()) {
            continue;
        }
        if (base >= region->base() && base + size < region->base() + region->size()) {
            usize preceding_size = base - region->base();
            usize succeeding_size = (region->base() + region->size()) - (base + size);
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
