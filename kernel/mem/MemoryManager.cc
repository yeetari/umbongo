#include <kernel/mem/MemoryManager.hh>

#include <boot/BootInfo.hh>
#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/Log.hh>
#include <ustd/Memory.hh>
#include <ustd/Optional.hh>
#include <ustd/Types.hh>

namespace {

constexpr usize k_bucket_bit_count = sizeof(usize) * 8;
constexpr usize k_frame_size = 4_KiB;

constexpr usize k_allocation_header_check = 0xdeadbeef;
struct AllocationHeader {
    usize check;
    usize size;
    Array<uint8, 0> data;
};

MemoryManager s_memory_manager;

} // namespace

void MemoryManager::initialise(BootInfo *boot_info) {
    new (&s_memory_manager) MemoryManager(boot_info);
}

MemoryManager::MemoryManager(BootInfo *boot_info) : m_boot_info(boot_info) {
    // Calculate end of physical memory.
    uintptr memory_end = 0;
    for (usize i = 0; i < boot_info->map_entry_count; i++) {
        auto &entry = boot_info->map[i];
        ENSURE(entry.base % k_frame_size == 0);

        const uintptr entry_end = entry.base + (entry.page_count * k_frame_size);
        if (entry_end > memory_end) {
            memory_end = entry_end;
        }
    }

    // Calculate total frame count.
    ASSERT(memory_end % k_frame_size == 0);
    m_total_frames = memory_end / k_frame_size;

    // Find some free memory for the physical frame bitset. We do this by finding the first fit entry in the memory map.
    const usize bucket_count = m_total_frames / k_bucket_bit_count;
    const auto bitset_location = find_first_fit_region(bucket_count * sizeof(usize));
    ENSURE(bitset_location);

    // Construct the frame bitset and then set reserved regions from the memory map.
    m_frame_bitset = new (reinterpret_cast<void *>(*bitset_location)) usize[bucket_count];
    for (usize i = 0; i < m_total_frames; i++) {
        set_frame(i);
    }
    for (usize i = 0; i < boot_info->map_entry_count; i++) {
        auto &entry = boot_info->map[i];
        if (entry.type != MemoryType::Available) {
            continue;
        }
        for (usize j = 0; j < entry.page_count; j++) {
            clear_frame(entry.base / k_frame_size + j);
        }
    }

    // Also mark the memory for the frame bitset itself as reserved.
    for (usize i = 0; i < round_up(bucket_count * sizeof(usize), k_frame_size) / k_frame_size; i++) {
        set_frame(*bitset_location / k_frame_size + i);
    }

    // Print some memory stats.
    usize free_bytes = 0;
    usize total_bytes = 0;
    for (usize i = 0; i < m_total_frames; i++) {
        free_bytes += !is_frame_set(i) ? k_frame_size : 0;
        total_bytes += k_frame_size;
    }
    logln(" mem: {}MiB/{}MiB free ({}%)", free_bytes / 1_MiB, total_bytes / 1_MiB, (free_bytes * 100) / total_bytes);
}

Optional<uintptr> MemoryManager::find_first_fit_region(usize size) {
    const usize page_count = round_up(size, k_frame_size) / k_frame_size;
    for (usize i = 0; i < m_boot_info->map_entry_count; i++) {
        auto &entry = m_boot_info->map[i];
        if (entry.type != MemoryType::Reserved && entry.page_count >= page_count) {
            return entry.base;
        }
    }
    return {};
}

usize &MemoryManager::frame_bitset_bucket(usize index) {
    ASSERT(index < m_total_frames);
    return m_frame_bitset[(index + k_bucket_bit_count) / k_bucket_bit_count];
}

void MemoryManager::clear_frame(usize index) {
    frame_bitset_bucket(index) &= ~(1ul << (index % k_bucket_bit_count));
}

void MemoryManager::set_frame(usize index) {
    frame_bitset_bucket(index) |= 1ul << (index % k_bucket_bit_count);
}

bool MemoryManager::is_frame_set(usize index) {
    return (frame_bitset_bucket(index) & (1ul << (index % k_bucket_bit_count))) != 0;
}

void *MemoryManager::alloc_phys(usize size) {
    ASSERT(m_boot_info != nullptr, "Attempted heap allocation before memory manager setup!");
    const usize frame_count = round_up(size, k_frame_size) / k_frame_size;
    for (usize i = 0; i < m_total_frames; i += frame_count) {
        bool found_space = true;
        for (usize j = i; j < i + frame_count; j++) {
            if (is_frame_set(j)) {
                found_space = false;
                break;
            }
        }

        if (!found_space) {
            continue;
        }

        for (usize j = i; j < i + frame_count; j++) {
            set_frame(j);
        }
        return reinterpret_cast<void *>(i * k_frame_size);
    }
    ENSURE_NOT_REACHED("No available physical memory!");
}

void MemoryManager::free_phys(void *ptr, usize size) {
    ASSERT(m_boot_info != nullptr, "Attempted heap deallocation before memory manager setup!");
    const auto first_frame = reinterpret_cast<uintptr>(ptr);
    ASSERT(first_frame % k_frame_size == 0);
    const auto first_frame_index = first_frame / k_frame_size;
    const usize frame_count = round_up(size, k_frame_size) / k_frame_size;
    for (usize i = first_frame_index; i < first_frame_index + frame_count; i++) {
        ASSERT(is_frame_set(i));
        clear_frame(i);
    }
}

void *operator new(usize size) {
    auto *header = static_cast<AllocationHeader *>(s_memory_manager.alloc_phys(size + sizeof(AllocationHeader)));
    header->check = k_allocation_header_check;
    header->size = size;
    return header->data.data();
}

void *operator new[](usize size) {
    return operator new(size);
}

void *operator new(usize size, std::align_val_t align) {
    const auto alignment = static_cast<usize>(align);
    auto *ptr = operator new(size + alignment);
    return reinterpret_cast<void *>(round_up(reinterpret_cast<uintptr>(ptr), alignment));
}

void *operator new[](usize size, std::align_val_t align) {
    return operator new(size, align);
}

void operator delete(void *ptr) {
    auto *header = reinterpret_cast<AllocationHeader *>(static_cast<uint8 *>(ptr) - sizeof(AllocationHeader));
    ASSERT(header->check == k_allocation_header_check);
    s_memory_manager.free_phys(header, header->size + sizeof(AllocationHeader));
}

void operator delete[](void *ptr) {
    return operator delete(ptr);
}

void operator delete(void *ptr, std::align_val_t align) {
    const auto alignment = static_cast<usize>(align);
    operator delete(reinterpret_cast<void *>(reinterpret_cast<uintptr>(ptr) - alignment + sizeof(AllocationHeader)));
}

void operator delete[](void *ptr, std::align_val_t align) {
    return operator delete(ptr, align);
}
