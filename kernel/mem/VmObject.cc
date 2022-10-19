#include <kernel/mem/VmObject.hh>

#include <kernel/proc/Process.hh>
#include <kernel/proc/Thread.hh>

namespace kernel {
namespace {

size_t from_page_size(PhysicalPageSize page_size) {
    switch (page_size) {
    case PhysicalPageSize::Normal:
        return 4_KiB;
    case PhysicalPageSize::Large:
        return 2_MiB;
    case PhysicalPageSize::Huge:
        return 1_GiB;
    }
}

PhysicalPageSize to_page_size(size_t size) {
    switch (size) {
    case 4_KiB:
        return PhysicalPageSize::Normal;
    case 2_MiB:
        return PhysicalPageSize::Large;
    case 1_GiB:
        return PhysicalPageSize::Huge;
    default:
        __builtin_unreachable();
    }
}

} // namespace

ustd::SharedPtr<VmObject> VmObject::create(size_t size) {
    size = ustd::align_up(size, 4_KiB);
    ustd::Vector<PhysicalPage> physical_pages;
    do {
        const auto next_size = size >= 1_GiB ? 1_GiB : size >= 2_MiB ? 2_MiB : 4_KiB;
        physical_pages.push(PhysicalPage::allocate(to_page_size(next_size)));
        size -= next_size;
    } while (size != 0);
    return ustd::make_shared<VmObject>(ustd::move(physical_pages));
}

ustd::SharedPtr<VmObject> VmObject::create_physical(uintptr_t base, size_t size) {
    size = ustd::align_up(size, 4_KiB);
    ustd::Vector<PhysicalPage> physical_pages;
    do {
        const auto next_size = size >= 1_GiB ? 1_GiB : size >= 2_MiB ? 2_MiB : 4_KiB;
        physical_pages.push(PhysicalPage::create(base, to_page_size(next_size)));
        base += next_size;
        size -= next_size;
    } while (size != 0);
    return ustd::make_shared<VmObject>(ustd::move(physical_pages));
}

size_t VmObject::size() const {
    size_t ret = 0;
    for (const auto &page : m_physical_pages) {
        ret += from_page_size(page.size());
    }
    return ret;
}

SyscallResult Thread::sys_vmo_create(size_t size) {
    return m_process->allocate_handle(VmObject::create(size));
}

SyscallResult Thread::sys_vmo_size(ub_handle_t handle) {
    auto vm_object = TRY(m_process->lookup_handle<VmObject>(handle));
    return vm_object->size();
}

} // namespace kernel
