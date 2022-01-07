#include <kernel/mem/Heap.hh>

#include <kernel/ScopedLock.hh> // IWYU pragma: keep
#include <kernel/SpinLock.hh>
#include <kernel/mem/MemoryManager.hh>
#include <ustd/Assert.hh>
#include <ustd/Memory.hh>
#include <ustd/Numeric.hh>
#include <ustd/Types.hh>

using namespace kernel;

namespace {

constexpr usize k_allocation_header_check = 0xdeadbeef;
constexpr usize k_bucket_bit_count = sizeof(usize) * 8;
constexpr usize k_chunk_size = 32;

struct AllocationHeader {
    usize check;
    usize chunk_count;
};

class Region {
    uint8 *const m_memory;
    usize *m_bitset;
    usize m_chunk_count;
    usize m_used_chunk_count{0};
    Region *m_next{nullptr};

public:
    Region(uint8 *memory, usize memory_size) : m_memory(memory) {
        m_chunk_count = memory_size / (k_chunk_size + 1);
        ASSERT(m_chunk_count * k_chunk_size + (m_chunk_count + 7) / 8 <= memory_size);
        m_bitset = reinterpret_cast<usize *>(memory + m_chunk_count * k_chunk_size);
        __builtin_memset(m_bitset, 0, m_chunk_count / 8);
    }

    void *allocate(usize size);
    void deallocate(void *ptr);

    bool contains(void *ptr) const;
    usize chunk_count() const { return m_chunk_count; }
    usize used_chunk_count() const { return m_used_chunk_count; }
    Region *&next() { return m_next; }
};

void *Region::allocate(usize size) {
    size += sizeof(AllocationHeader);
    usize desired_chunk_count = (size + k_chunk_size - 1) / k_chunk_size;
    usize free_chunk_count = 0;
    usize first_chunk = 0;
    for (usize bucket_index = 0; bucket_index < m_chunk_count / k_bucket_bit_count; bucket_index++) {
        if (free_chunk_count >= desired_chunk_count) {
            break;
        }
        usize bucket = m_bitset[bucket_index];
        if (bucket == 0) {
            if (free_chunk_count == 0) {
                first_chunk = bucket_index * k_bucket_bit_count;
            }
            free_chunk_count += k_bucket_bit_count;
            continue;
        }
        if (bucket == ustd::Limits<usize>::max()) {
            free_chunk_count = 0;
            continue;
        }
        for (usize viewed_bit_count = 0; viewed_bit_count < k_bucket_bit_count;) {
            if (bucket == 0) {
                if (free_chunk_count == 0) {
                    first_chunk = bucket_index * k_bucket_bit_count + viewed_bit_count;
                }
                free_chunk_count += k_bucket_bit_count - viewed_bit_count;
                viewed_bit_count += k_bucket_bit_count;
                continue;
            }
            const auto trailing_zeroes = static_cast<usize>(__builtin_ctzll(bucket));
            bucket >>= trailing_zeroes;
            if (free_chunk_count == 0) {
                first_chunk = bucket_index * k_bucket_bit_count + viewed_bit_count;
            }
            free_chunk_count += trailing_zeroes;
            viewed_bit_count += trailing_zeroes;
            if (free_chunk_count >= desired_chunk_count) {
                break;
            }
            const auto trailing_ones = static_cast<usize>(__builtin_ctzll(~bucket));
            bucket >>= trailing_ones;
            viewed_bit_count += trailing_ones;
            free_chunk_count = 0;
        }
    }
    if (free_chunk_count < desired_chunk_count) {
        return nullptr;
    }
    auto *header = reinterpret_cast<AllocationHeader *>(m_memory + first_chunk * k_chunk_size);
    header->check = k_allocation_header_check;
    header->chunk_count = desired_chunk_count;
    for (usize i = first_chunk; i < first_chunk + desired_chunk_count; i++) {
        auto &bucket = m_bitset[(i + k_bucket_bit_count) / k_bucket_bit_count - 1];
        ASSERT((bucket & (1ul << (i % k_bucket_bit_count))) == 0);
        bucket |= 1ul << (i % k_bucket_bit_count);
    }
    m_used_chunk_count += desired_chunk_count;
    return &header[1];
}

void Region::deallocate(void *ptr) {
    auto *header = &reinterpret_cast<AllocationHeader *>(ptr)[-1];
    ASSERT(header->check == k_allocation_header_check);
    const auto first_chunk = (reinterpret_cast<uintptr>(header) - reinterpret_cast<uintptr>(m_memory)) / k_chunk_size;
    for (usize i = first_chunk; i < first_chunk + header->chunk_count; i++) {
        auto &bucket = m_bitset[(i + k_bucket_bit_count) / k_bucket_bit_count - 1];
        ASSERT((bucket & (1ul << (i % k_bucket_bit_count))) != 0);
        bucket &= ~(1ul << (i % k_bucket_bit_count));
    }
    m_used_chunk_count -= header->chunk_count;
}

bool Region::contains(void *ptr) const {
    auto *header = &reinterpret_cast<AllocationHeader *>(ptr)[-1];
    ASSERT(header->check == k_allocation_header_check);
    if (reinterpret_cast<uint8 *>(header) < m_memory) {
        return false;
    }
    if (reinterpret_cast<uint8 *>(ptr) >= m_memory + m_chunk_count * k_chunk_size) {
        return false;
    }
    return true;
}

Region *s_base_region = nullptr;
SpinLock s_lock;

Region *create_region() {
    constexpr usize aligned_size = ustd::round_up(sizeof(Region), 16);
    auto *memory = MemoryManager::alloc_contiguous(1_MiB);
    auto *region = new (memory) Region(reinterpret_cast<uint8 *>(memory) + aligned_size, 1_MiB - aligned_size);
    Region *last_region = nullptr;
    for (auto *reg = s_base_region; reg != nullptr; reg = reg->next()) {
        last_region = reg;
    }
    ASSERT(last_region != nullptr && last_region->next() == nullptr);
    last_region->next() = region;
    return region;
}

void *allocate(usize size) {
    if (size >= s_base_region->chunk_count() * k_chunk_size - 1_KiB) {
        auto *memory = MemoryManager::alloc_contiguous(size + sizeof(AllocationHeader));
        auto *header = reinterpret_cast<AllocationHeader *>(memory);
        header->check = k_allocation_header_check;
        header->chunk_count = (size + k_chunk_size - 1) / k_chunk_size;
        return &header[1];
    }
    for (auto *region = s_base_region; region != nullptr; region = region->next()) {
        if (auto *ptr = region->allocate(size)) {
            return ptr;
        }
    }
    return create_region()->allocate(size);
}

void deallocate(void *ptr) {
    auto *header = &reinterpret_cast<AllocationHeader *>(ptr)[-1];
    ASSERT(header->check == k_allocation_header_check);
    if (header->chunk_count * k_chunk_size > s_base_region->chunk_count() * k_chunk_size - 1_KiB) {
        MemoryManager::free_contiguous(header, header->chunk_count * k_chunk_size);
        return;
    }
    for (Region *region = s_base_region, *previous_region = nullptr; region != nullptr; region = region->next()) {
        if (region->contains(ptr)) {
            region->deallocate(ptr);
            if (region->used_chunk_count() == 0 && region != s_base_region) {
                previous_region->next() = region->next();
                region->~Region();
                MemoryManager::free_contiguous(region, 1_MiB);
            }
            return;
        }
        previous_region = region;
    }
}

} // namespace

void Heap::initialise() {
    ASSERT(s_base_region == nullptr);
    constexpr usize aligned_size = ustd::round_up(sizeof(Region), 16);
    auto *memory = MemoryManager::alloc_contiguous(1_MiB);
    s_base_region = new (memory) Region(reinterpret_cast<uint8 *>(memory) + aligned_size, 1_MiB - aligned_size);
}

void *operator new(usize size) {
    ScopedLock locker(s_lock);
    return allocate(size);
}

void *operator new[](usize size) {
    return operator new(size);
}

void *operator new(usize size, ustd::align_val_t align) {
    const auto alignment = static_cast<usize>(align);
    ASSERT(alignment != 0);
    ScopedLock locker(s_lock);
    void *ptr = allocate(size + alignment + sizeof(ptrdiff));
    usize max_addr = reinterpret_cast<usize>(ptr) + alignment;
    void *aligned_ptr = reinterpret_cast<void *>(max_addr - (max_addr % alignment));
    (reinterpret_cast<ptrdiff *>(aligned_ptr))[-1] =
        reinterpret_cast<uint8 *>(aligned_ptr) - reinterpret_cast<uint8 *>(ptr);
    return aligned_ptr;
}

void *operator new[](usize size, ustd::align_val_t align) {
    return operator new(size, align);
}

void operator delete(void *ptr) {
    if (ptr == nullptr) {
        return;
    }
    ScopedLock locker(s_lock);
    deallocate(ptr);
}

void operator delete[](void *ptr) {
    return operator delete(ptr);
}

void operator delete(void *ptr, ustd::align_val_t) {
    if (ptr == nullptr) {
        return;
    }
    ScopedLock locker(s_lock);
    deallocate(reinterpret_cast<uint8 *>(ptr) - (reinterpret_cast<const ptrdiff *>(ptr))[-1]);
}

void operator delete[](void *ptr, ustd::align_val_t align) {
    return operator delete(ptr, align);
}
