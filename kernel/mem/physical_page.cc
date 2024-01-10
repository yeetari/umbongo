#include <kernel/mem/physical_page.hh>

#include <kernel/mem/memory_manager.hh>
#include <ustd/types.hh>

namespace kernel {

PhysicalPage PhysicalPage::allocate(PhysicalPageSize size) {
    switch (size) {
    case PhysicalPageSize::Normal:
        return {MemoryManager::alloc_frame(), size, true};
    case PhysicalPageSize::Large: {
        const auto phys = reinterpret_cast<uintptr_t>(MemoryManager::alloc_contiguous(2_MiB));
        return {phys, size, true};
    }
    case PhysicalPageSize::Huge:
        const auto phys = reinterpret_cast<uintptr_t>(MemoryManager::alloc_contiguous(1_GiB));
        return {phys, size, true};
    }
}

PhysicalPage PhysicalPage::create(uintptr_t phys, PhysicalPageSize size) {
    return {phys, size, false};
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
