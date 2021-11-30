#include <kernel/mem/PhysicalPage.hh>

#include <kernel/mem/MemoryManager.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Types.hh>

SharedPtr<PhysicalPage> PhysicalPage::allocate(PhysicalPageSize size) {
    switch (size) {
    case PhysicalPageSize::Normal:
        return ustd::make_shared<PhysicalPage>(MemoryManager::alloc_frame(), size);
    case PhysicalPageSize::Large: {
        const auto phys = reinterpret_cast<uintptr>(MemoryManager::alloc_contiguous(2_MiB));
        return ustd::make_shared<PhysicalPage>(phys, size);
    }
    case PhysicalPageSize::Huge:
        const auto phys = reinterpret_cast<uintptr>(MemoryManager::alloc_contiguous(1_GiB));
        return ustd::make_shared<PhysicalPage>(phys, size);
    }
}

SharedPtr<PhysicalPage> PhysicalPage::create(uintptr phys, PhysicalPageSize size) {
    return ustd::make_shared<PhysicalPage>(phys, size);
}

PhysicalPage::~PhysicalPage() {
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
