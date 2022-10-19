#include <kernel/mem/VirtSpace.hh>

#include <kernel/ScopedLock.hh> // IWYU pragma: keep
#include <kernel/cpu/Paging.hh>
#include <kernel/mem/Region.hh>
#include <kernel/mem/VirtualRange.hh>
#include <kernel/mem/VmObject.hh>
#include <kernel/proc/Process.hh>
#include <kernel/proc/Thread.hh>
#include <ustd/Assert.hh>
#include <ustd/Numeric.hh>
#include <ustd/Optional.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Try.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>
#include <ustd/Utility.hh>
#include <ustd/Vector.hh>

namespace kernel {
namespace {

constexpr size_t k_total_size = 1ull << 48u;

} // namespace

VirtSpace::VirtSpace() : m_pml4(new Pml4) {}

VirtSpace::~VirtSpace() {
    // TODO: This could be made much more efficient by not having to call Region's destructor (which needs to take the
    //       lock each time, remove itself from the tree and unmap its pages from the page directory unnecessarily).

    // Free any inherited regions.
    m_inherited_regions.clear();
    ASSERT(m_region_tree.root_node() == nullptr);
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

void VirtSpace::unmap_4KiB(uintptr_t virt) {
    ASSERT_PEDANTIC(virt % 4_KiB == 0);
    const size_t pml4_index = (virt >> 39ul) & 0x1fful;
    const size_t pdpt_index = (virt >> 30ul) & 0x1fful;
    const size_t pd_index = (virt >> 21ul) & 0x1fful;
    const size_t pt_index = (virt >> 12ul) & 0x1fful;
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

void VirtSpace::unmap_2MiB(uintptr_t virt) {
    ASSERT_PEDANTIC(virt % 2_MiB == 0);
    const size_t pml4_index = (virt >> 39ul) & 0x1fful;
    const size_t pdpt_index = (virt >> 30ul) & 0x1fful;
    const size_t pd_index = (virt >> 21ul) & 0x1fful;
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

void VirtSpace::unmap_1GiB(uintptr_t virt) {
    ASSERT_PEDANTIC(virt % 1_GiB == 0);
    const size_t pml4_index = (virt >> 39ul) & 0x1fful;
    const size_t pdpt_index = (virt >> 30ul) & 0x1fful;
    auto *pdpt = m_pml4->expect(pml4_index);
    pdpt->unset(pdpt_index);
    if (pdpt->entry_count() == 0u) {
        m_pml4->unset(pml4_index);
    }
}

void VirtSpace::detree_region(Region *region) {
    ScopedLock lock(m_lock);
    m_region_tree.remove(region);
}

SysResult<ustd::SharedPtr<Region>> VirtSpace::allocate_anywhere(RegionAccess access, size_t size) {
    const auto alignment = size >= 1_GiB ? 1_GiB : size >= 2_MiB ? 2_MiB : 4_KiB;

    auto try_allocate_in_gap = [&](VirtualRange range) -> ustd::SharedPtr<Region> {
        // Ignore gaps too small and gaps out of range.
        if (range.size() < size || range.end() > k_total_size) {
            return {};
        }
        auto *region = new Region(*this, {range.base(), size}, access);
        ASSERT(m_region_tree.find_no_splay(region->base()) == nullptr);
        m_region_tree.insert(region);
        ASSERT(m_region_tree.find_no_splay(region->base()) == region);
        return ustd::adopt_shared(region);
    };

    ScopedLock lock(m_lock);
    uintptr_t prev_region_end = alignment;
    for (auto *region = m_region_tree.minimum_node(); region != nullptr; region = RegionTree::successor(region)) {
        if (region->base() > prev_region_end) {
            const size_t gap_size = region->base() - prev_region_end;
            if (auto new_region = try_allocate_in_gap({prev_region_end, gap_size})) {
                return new_region;
            }
        }
        // TODO: Should check overflow.
        prev_region_end = ustd::align_up(region->range().end(), alignment);
    }

    // No more regions, but there is likely still a gap after the last region covering the rest of the unallocated
    // virtual space.
    if (prev_region_end < k_total_size) {
        if (auto region = try_allocate_in_gap({prev_region_end, k_total_size - prev_region_end})) {
            return region;
        }
    }
    return Error::VirtSpaceFull;
}

SysResult<ustd::SharedPtr<Region>> VirtSpace::allocate_anywhere(RegionAccess access,
                                                                ustd::SharedPtr<VmObject> vm_object) {
    auto region = TRY(allocate_anywhere(access, vm_object->size()));
    region->map(ustd::move(vm_object));
    return region;
}

SysResult<ustd::SharedPtr<Region>> VirtSpace::allocate_specific(uintptr_t base, size_t size, RegionAccess access) {
    // TODO: Proper validation.
    ScopedLock lock(m_lock);
    ASSERT(m_region_tree.find_no_splay(base) == nullptr);
    auto *region = new Region(*this, {base, size}, access);
    m_region_tree.insert(region);
    return ustd::adopt_shared(region);
}

SysResult<ustd::SharedPtr<Region>> VirtSpace::allocate_specific(uintptr_t base, RegionAccess access,
                                                                ustd::SharedPtr<VmObject> vm_object) {
    auto region = TRY(allocate_specific(base, vm_object->size(), access));
    region->map(ustd::move(vm_object));
    return region;
}

ustd::SharedPtr<VirtSpace> VirtSpace::clone() const {
    auto new_space = ustd::make_shared<VirtSpace>();
    ScopedLock lock(m_lock);
    for (auto *region = m_region_tree.minimum_node(); region != nullptr; region = RegionTree::successor(region)) {
        auto *region_copy = new Region(*new_space, region->range(), region->access());
        new_space->m_region_tree.insert(region_copy);
        region_copy->map(region->vm_object());
        new_space->m_inherited_regions.push(ustd::adopt_shared(region_copy));
    }
    return new_space;
}

SyscallResult Thread::sys_vmr_allocate(size_t size, UserPtr<uintptr_t> out_address) {
    // TODO: Configurable access.
    constexpr auto access = RegionAccess::UserAccessible | RegionAccess::Writable;
    auto region = TRY(m_process->virt_space().allocate_anywhere(access, size));
    if (out_address) {
        auto base = region->base();
        TRY(copy_to_user(out_address, &base, 1));
    }
    return m_process->allocate_handle(ustd::move(region));
}

SyscallResult Thread::sys_vmr_map(ub_handle_t vmr_handle, ub_handle_t vmo_handle) {
    auto region = TRY(m_process->lookup_handle<Region>(vmr_handle));
    auto vm_object = TRY(m_process->lookup_handle<VmObject>(vmo_handle));
    region->map(ustd::move(vm_object));
    return 0;
}

} // namespace kernel
