#include <kernel/mem/PhysicalPage.hh>

#include <kernel/mem/MemoryManager.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Types.hh>

SharedPtr<PhysicalPage> PhysicalPage::allocate() {
    auto phys = MemoryManager::alloc_frame();
    return ustd::make_shared<PhysicalPage>(phys, PhysicalPageSize::Normal);
}

SharedPtr<PhysicalPage> PhysicalPage::create(uintptr phys, PhysicalPageSize size) {
    return ustd::make_shared<PhysicalPage>(phys, size);
}

PhysicalPage::~PhysicalPage() {
    MemoryManager::free_frame(phys());
}
