#include <kernel/mem/address_space.hh>

#include <kernel/dmesg.hh>
#include <kernel/error.hh>
#include <kernel/mem/region.hh>
#include <kernel/mem/vm_object.hh>
#include <kernel/proc/process.hh>
#include <kernel/scoped_lock.hh>
#include <ustd/assert.hh>
#include <ustd/try.hh>

namespace kernel {
namespace {

// TODO: Arch specific.
constexpr size_t k_total_size = 1ull << 48u;

} // namespace

AddressSpace::AddressSpace(Process &process) : m_process(process), m_pml4(new Pml4) {}

AddressSpace::~AddressSpace() {
    m_regions.clear();
    ASSERT(m_region_tree.root_node() == nullptr);
}

void AddressSpace::detree_region(Region *region) {
    ScopedLock lock(m_lock);
    m_region_tree.remove(region);
}

void AddressSpace::map_4KiB(uintptr_t virt, uintptr_t phys, PageFlags flags) {
    ASSERT(virt % 4_KiB == 0);
    ASSERT(phys % 4_KiB == 0);
    const size_t pml4_index = (virt >> 39ul) & 0x1fful;
    const size_t pdpt_index = (virt >> 30ul) & 0x1fful;
    const size_t pd_index = (virt >> 21ul) & 0x1fful;
    const size_t pt_index = (virt >> 12ul) & 0x1fful;

    ScopedLock lock(m_lock);
    auto *pdpt = m_pml4->ensure(pml4_index);
    auto *pd = pdpt->ensure(pdpt_index);
    auto *pt = pd->ensure(pd_index);
    pt->set(pt_index, phys, flags);
}

void AddressSpace::map_2MiB(uintptr_t virt, uintptr_t phys, PageFlags flags) {
    ASSERT(virt % 2_MiB == 0);
    ASSERT(phys % 2_MiB == 0);
    const size_t pml4_index = (virt >> 39ul) & 0x1fful;
    const size_t pdpt_index = (virt >> 30ul) & 0x1fful;
    const size_t pd_index = (virt >> 21ul) & 0x1fful;

    ScopedLock lock(m_lock);
    auto *pdpt = m_pml4->ensure(pml4_index);
    auto *pd = pdpt->ensure(pdpt_index);
    pd->set(pd_index, phys, flags | PageFlags::Large);
}

void AddressSpace::map_1GiB(uintptr_t virt, uintptr_t phys, PageFlags flags) {
    ASSERT(virt % 1_GiB == 0);
    ASSERT(phys % 1_GiB == 0);
    const size_t pml4_index = (virt >> 39ul) & 0x1fful;
    const size_t pdpt_index = (virt >> 30ul) & 0x1fful;

    ScopedLock lock(m_lock);
    auto *pdpt = m_pml4->ensure(pml4_index);
    pdpt->set(pdpt_index, phys, flags | PageFlags::Large);
}

void AddressSpace::unmap_4KiB(uintptr_t virt) {
    ASSERT(virt % 4_KiB == 0);
    const size_t pml4_index = (virt >> 39ul) & 0x1fful;
    const size_t pdpt_index = (virt >> 30ul) & 0x1fful;
    const size_t pd_index = (virt >> 21ul) & 0x1fful;
    const size_t pt_index = (virt >> 12ul) & 0x1fful;

    ScopedLock lock(m_lock);
    auto *pdpt = m_pml4->expect(pml4_index);
    auto *pd = pdpt->expect(pdpt_index);
    auto *pt = pd->expect(pd_index);
    pt->unset(pt_index);
    if (pt->entry_count() == 0u) {
        pd->unset(pd_index);
    }
    if (pd->entry_count() == 0u) {
        pdpt->unset(pdpt_index);
    }
    if (pdpt->entry_count() == 0u) {
        m_pml4->unset(pml4_index);
    }
}

void AddressSpace::unmap_2MiB(uintptr_t virt) {
    ASSERT(virt % 2_MiB == 0);
    const size_t pml4_index = (virt >> 39ul) & 0x1fful;
    const size_t pdpt_index = (virt >> 30ul) & 0x1fful;
    const size_t pd_index = (virt >> 21ul) & 0x1fful;

    ScopedLock lock(m_lock);
    auto *pdpt = m_pml4->expect(pml4_index);
    auto *pd = pdpt->expect(pdpt_index);
    pd->unset(pd_index);
    if (pd->entry_count() == 0u) {
        pdpt->unset(pdpt_index);
    }
    if (pdpt->entry_count() == 0u) {
        m_pml4->unset(pml4_index);
    }
}

void AddressSpace::unmap_1GiB(uintptr_t virt) {
    ASSERT(virt % 1_GiB == 0);
    const size_t pml4_index = (virt >> 39ul) & 0x1fful;
    const size_t pdpt_index = (virt >> 30ul) & 0x1fful;

    ScopedLock lock(m_lock);
    auto *pdpt = m_pml4->expect(pml4_index);
    pdpt->unset(pdpt_index);
    if (pdpt->entry_count() == 0u) {
        m_pml4->unset(pml4_index);
    }
}

SysResult<VirtualRange> AddressSpace::allocate_range_anywhere(size_t size) {
    ASSERT(size % 4_KiB == 0);
    const auto alignment = size >= 1_GiB ? 1_GiB : size >= 2_MiB ? 2_MiB : 4_KiB;

    uintptr_t previous_region_end = alignment;
    for (auto *region = m_region_tree.minimum_node(); region != nullptr; region = RegionTree::successor(region)) {
        if (region->base() > previous_region_end) {
            VirtualRange available_range(previous_region_end, region->base() - previous_region_end);
            if (available_range.size() >= size && available_range.end() < k_total_size) {
                return VirtualRange(available_range.base(), size);
            }
        }

        // TODO: Should check for overflow.
        previous_region_end = ustd::align_up(region->range().end(), alignment);
    }

    // No suitable gaps, but there is likely still a large gap after the last region covering the rest of unallocated
    // address space.
    if (previous_region_end < k_total_size) {
        const auto available_size = k_total_size - previous_region_end;
        if (available_size >= size) {
            return VirtualRange(previous_region_end, size);
        }
    }

    // TODO: Return error.
    ENSURE_NOT_REACHED();
}

SysResult<VirtualRange> AddressSpace::allocate_range_specific(VirtualRange range) {
    if (m_region_tree.find_no_splay(range.base()) != nullptr) {
        // TODO
    }
    // TODO: Check size.
    return range;
}

Region &AddressSpace::new_region(VirtualRange range, RegionAccess access) {
    auto &region = *m_regions.emplace(new Region(*this, range, access));
    m_region_tree.insert(&region);
    return region;
}

SysResult<Region &> AddressSpace::allocate_anywhere(size_t size, RegionAccess access) {
    ScopedLock lock(m_lock);
    const auto range = TRY(allocate_range_anywhere(size));
    return new_region(range, access);
}

SysResult<Region &> AddressSpace::allocate_specific(VirtualRange range, RegionAccess access) {
    ScopedLock lock(m_lock);
    range = TRY(allocate_range_specific(range));
    return new_region(range, access);
}

SysResult<uintptr_t> AddressSpace::virt_to_phys(uintptr_t virt) {
    // TODO(rb-tree): This sucks.
    ScopedLock lock(m_lock);
    for (auto *region = m_region_tree.minimum_node(); region != nullptr; region = RegionTree::successor(region)) {
        if (virt < region->base() || virt >= region->range().end()) {
            continue;
        }
        for (uintptr_t page_virt = region->base(); const auto &physical_page : region->vm_object()->physical_pages()) {
            uintptr_t page_end = page_virt;
            switch (physical_page.size()) {
            case PhysicalPageSize::Normal:
                page_end += 4_KiB;
                break;
            case PhysicalPageSize::Large:
                page_end += 2_MiB;
                break;
            case PhysicalPageSize::Huge:
                page_end += 1_GiB;
                break;
            }
            if (virt < page_virt || virt >= page_end) {
                continue;
            }
            size_t offset = virt - page_virt;
            switch (physical_page.size()) {
            case PhysicalPageSize::Normal:
                page_virt += 4_KiB;
                break;
            case PhysicalPageSize::Large:
                page_virt += 2_MiB;
                break;
            case PhysicalPageSize::Huge:
                page_virt += 1_GiB;
                break;
            }
            return physical_page.phys() + offset;
        }
    }
    return Error::NonExistent;
}

} // namespace kernel
