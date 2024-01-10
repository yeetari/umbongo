#include <kernel/mem/region.hh>

#include <kernel/arch/cpu.hh>
#include <kernel/arch/paging.hh>
#include <kernel/mem/address_space.hh>
#include <kernel/mem/vm_object.hh>
#include <kernel/proc/process.hh>

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

Region::Region(AddressSpace &address_space, VirtualRange range, RegionAccess access)
    : m_address_space(address_space), m_range(range), m_access(access) {}

Region::~Region() {
    m_address_space.detree_region(this);
    unmap_if_needed();
}

void Region::map(ustd::SharedPtr<VmObject> &&vm_object) {
    ASSERT(!m_vm_object);
    m_vm_object = ustd::move(vm_object);

    ASSERT(m_vm_object->size() <= m_range.size());

    const auto flags = page_flags(m_access);
    for (uintptr_t virt = m_range.base(); const auto &physical_page : m_vm_object->physical_pages()) {
        switch (physical_page.size()) {
        case PhysicalPageSize::Normal:
            m_address_space.map_4KiB(virt, physical_page.phys(), flags);
            virt += 4_KiB;
            break;
        case PhysicalPageSize::Large:
            m_address_space.map_2MiB(virt, physical_page.phys(), flags);
            virt += 2_MiB;
            break;
        case PhysicalPageSize::Huge:
            m_address_space.map_1GiB(virt, physical_page.phys(), flags);
            virt += 1_GiB;
            break;
        }
    }

    // TODO: Lazy TLB invalidation - if not flushing the TLB would cause a page fault next access, we can leave it and
    //       let the page fault handler invalidate the TLB. We can do this for mapping a new VM object (not present ->
    //       present), but also for any permission increase (e.g. read only -> writable/executable).
    arch::tlb_flush_range(m_address_space, m_range);
}

void Region::unmap_if_needed() {
    if (!m_vm_object) {
        return;
    }

    for (uintptr_t virt = m_range.base(); const auto &physical_page : m_vm_object->physical_pages()) {
        switch (physical_page.size()) {
        case PhysicalPageSize::Normal:
            m_address_space.unmap_4KiB(virt);
            virt += 4_KiB;
            break;
        case PhysicalPageSize::Large:
            m_address_space.unmap_2MiB(virt);
            virt += 2_MiB;
            break;
        case PhysicalPageSize::Huge:
            m_address_space.unmap_1GiB(virt);
            virt += 1_GiB;
            break;
        }
    }

    m_vm_object.clear();
    arch::tlb_flush_range(m_address_space, m_range);
}

ustd::SharedPtr<VmObject> Region::vm_object() const {
    return m_vm_object;
}

} // namespace kernel
