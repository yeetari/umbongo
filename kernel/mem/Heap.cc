#include <kernel/mem/MemoryManager.hh>
#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/Memory.hh>
#include <ustd/Types.hh>

namespace {

constexpr usize k_allocation_header_check = 0xdeadbeef;
struct AllocationHeader {
    usize check;
    usize size;
    void *mem;
    Array<uint8, 0> data;
};

} // namespace

void *operator new(usize size) {
    auto *header = static_cast<AllocationHeader *>(MemoryManager::alloc_contiguous(size + sizeof(AllocationHeader)));
    header->check = k_allocation_header_check;
    header->size = size;
    return header->data.data();
}

void *operator new[](usize size) {
    return operator new(size);
}

void *operator new(usize size, ustd::align_val_t align) {
    const auto alignment = static_cast<usize>(align);
    ASSERT(alignment != 0);
    void *unaligned_ptr = MemoryManager::alloc_contiguous(size + alignment + sizeof(AllocationHeader));
    auto unaligned = reinterpret_cast<uintptr>(unaligned_ptr) + sizeof(AllocationHeader);
    uintptr aligned = round_up(unaligned, alignment);
    auto *header = reinterpret_cast<AllocationHeader *>(aligned - sizeof(AllocationHeader));
    ASSERT(reinterpret_cast<uintptr>(header) >= reinterpret_cast<uintptr>(unaligned_ptr));
    header->check = k_allocation_header_check;
    header->size = size;
    header->mem = unaligned_ptr;
    ASSERT(reinterpret_cast<uintptr>(header->data.data() + size) <
           reinterpret_cast<uintptr>(unaligned_ptr) + size + alignment + sizeof(AllocationHeader));
    return header->data.data();
}

void *operator new[](usize size, ustd::align_val_t align) {
    return operator new(size, align);
}

// TODO: We need this noinline attribute because something is really wrong when compiling this function with LTO.
[[gnu::noinline]] void operator delete(void *ptr) noexcept {
    if (ptr == nullptr) {
        return;
    }
    auto *header = reinterpret_cast<AllocationHeader *>(static_cast<uint8 *>(ptr) - sizeof(AllocationHeader));
    ASSERT(header->check == k_allocation_header_check);
    MemoryManager::free_contiguous(header, header->size + sizeof(AllocationHeader));
}

void operator delete[](void *ptr) noexcept {
    return operator delete(ptr);
}

void operator delete(void *ptr, ustd::align_val_t align) noexcept {
    if (ptr == nullptr) {
        return;
    }
    const auto alignment = static_cast<usize>(align);
    auto *header = reinterpret_cast<AllocationHeader *>(reinterpret_cast<uintptr>(ptr) - sizeof(AllocationHeader));
    ASSERT(header->check == k_allocation_header_check);
    ASSERT(header->mem != nullptr);
    MemoryManager::free_contiguous(header->mem, header->size + alignment + sizeof(AllocationHeader));
}

void operator delete[](void *ptr, ustd::align_val_t align) noexcept {
    return operator delete(ptr, align);
}
