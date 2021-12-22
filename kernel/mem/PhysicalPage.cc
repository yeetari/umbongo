#include <kernel/mem/PhysicalPage.hh>

#include <kernel/mem/MemoryManager.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Types.hh>

namespace kernel {

ustd::SharedPtr<PhysicalPage> PhysicalPage::allocate(PhysicalPageSize size) {
    switch (size) {
    case PhysicalPageSize::Normal:
        return ustd::make_shared<PhysicalPage>(MemoryManager::alloc_frame(), size, true);
    case PhysicalPageSize::Large: {
        const auto phys = reinterpret_cast<uintptr>(MemoryManager::alloc_contiguous(2_MiB));
        return ustd::make_shared<PhysicalPage>(phys, size, true);
    }
    case PhysicalPageSize::Huge:
        const auto phys = reinterpret_cast<uintptr>(MemoryManager::alloc_contiguous(1_GiB));
        return ustd::make_shared<PhysicalPage>(phys, size, true);
    }
}

ustd::SharedPtr<PhysicalPage> PhysicalPage::create(uintptr phys, PhysicalPageSize size) {
    return ustd::make_shared<PhysicalPage>(phys, size, false);
}

PhysicalPage::~PhysicalPage() {
    if (!allocated()) {
        return;
    }
    switch (size()) {
    case PhysicalPageSize::Normal:
        MemoryManager::free_frame(phys());
        break;
    case PhysicalPageSize::Large:
        MemoryManager::free_contiguous(reinterpret_cast<void *>(phys()), 2_MiB);
        break;
    case PhysicalPageSize::Huge:
        MemoryManager::free_contiguous(reinterpret_cast<void *>(phys()), 1_GiB);
        break;
    }
}

} // namespace kernel
