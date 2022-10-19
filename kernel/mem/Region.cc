#include <kernel/mem/Region.hh>

#include <kernel/cpu/Paging.hh>
#include <kernel/mem/MemoryManager.hh>
#include <kernel/mem/PhysicalPage.hh>
#include <kernel/mem/VirtSpace.hh>
#include <kernel/mem/VmObject.hh>
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

Region::Region(VirtSpace &virt_space, VirtualRange range, RegionAccess access)
    : m_virt_space(virt_space), m_range(range), m_access(access) {
}

Region::~Region() {
    m_virt_space.detree_region(this);
    unmap_if_needed();
}

void Region::map(ustd::SharedPtr<VmObject> &&vm_object) {
    unmap_if_needed();

    m_vm_object = ustd::move(vm_object);
    const auto flags = page_flags(m_access);
    for (uintptr_t virt = m_range.base(); const auto &physical_page : m_vm_object->physical_pages()) {
        switch (physical_page.size()) {
        case PhysicalPageSize::Normal:
            m_virt_space.map_4KiB(virt, physical_page.phys(), flags);
            virt += 4_KiB;
            break;
        case PhysicalPageSize::Large:
            m_virt_space.map_2MiB(virt, physical_page.phys(), flags);
            virt += 2_MiB;
            break;
        case PhysicalPageSize::Huge:
            m_virt_space.map_1GiB(virt, physical_page.phys(), flags);
            virt += 1_GiB;
            break;
        }
    }

    if (MemoryManager::current_space() == &m_virt_space) {
        for (uintptr_t virt = m_range.base(); virt < m_range.end(); virt += 4_KiB) {
            asm volatile("invlpg (%0)" : : "r"(virt) : "memory");
        }
    }
}

void Region::unmap_if_needed() {
    if (!m_vm_object) {
        // Nothing mapped.
        return;
    }

    for (uintptr_t virt = m_range.base(); const auto &physical_page : m_vm_object->physical_pages()) {
        switch (physical_page.size()) {
        case PhysicalPageSize::Normal:
            m_virt_space.unmap_4KiB(virt);
            virt += 4_KiB;
            break;
        case PhysicalPageSize::Large:
            m_virt_space.unmap_2MiB(virt);
            virt += 2_MiB;
            break;
        case PhysicalPageSize::Huge:
            m_virt_space.unmap_1GiB(virt);
            virt += 1_GiB;
            break;
        }
    }
    m_vm_object.clear();

    if (MemoryManager::current_space() == &m_virt_space) {
        for (uintptr_t virt = m_range.base(); virt < m_range.end(); virt += 4_KiB) {
            asm volatile("invlpg (%0)" : : "r"(virt) : "memory");
        }
    }
}

ustd::SharedPtr<VmObject> Region::vm_object() const {
    return m_vm_object;
}

} // namespace kernel
