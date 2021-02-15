#include <kernel/mem/MemoryManager.hh>

#include <boot/BootInfo.hh>
#include <ustd/Assert.hh>
#include <ustd/Log.hh>
#include <ustd/Memory.hh>
#include <ustd/Optional.hh>
#include <ustd/Types.hh>

namespace {

constexpr usize k_bucket_bit_count = sizeof(usize) * 8;
constexpr usize k_page_size = 4096;

constexpr usize page_round_up(usize x) {
    return (x + k_page_size - 1) & ~(k_page_size - 1);
}

} // namespace

MemoryManager::MemoryManager(BootInfo *boot_info) : m_boot_info(boot_info) {
    // Count memory.
    usize free_bytes = 0;
    usize total_bytes = 0;
    for (usize i = 0; i < boot_info->map_entry_count; i++) {
        auto &entry = boot_info->map[i];
        ENSURE(entry.base % k_page_size == 0);

        const usize entry_size = entry.page_count * k_page_size;
        free_bytes += entry.type != MemoryType::Reserved ? entry_size : 0;
        total_bytes += entry_size;
        m_total_frames += entry.page_count;
    }
    log(" mem: {}MiB/{}MiB free ({}%)", free_bytes / 1_MiB, total_bytes / 1_MiB, (free_bytes * 100) / total_bytes);

    // Find some free memory for the physical frame bitset. We do this by finding the first fit entry in the memory map.
    const usize bucket_count = m_total_frames / k_bucket_bit_count;
    const auto bitset_location = find_first_fit_region(bucket_count * sizeof(usize));
    ENSURE(bitset_location);

    // Construct the frame bitset and then set reserved regions from the memory map.
    m_frame_bitset = new (reinterpret_cast<void *>(*bitset_location)) usize[bucket_count];
    for (usize i = 0; i < boot_info->map_entry_count; i++) {
        auto &entry = boot_info->map[i];
        if (entry.type == MemoryType::Available) {
            continue;
        }
        for (usize j = 0; j < entry.page_count; j++) {
            set_frame(entry.base / k_page_size + j);
        }
    }

    // Also mark the memory for the frame bitset itself as reserved.
    for (usize i = 0; i < page_round_up(bucket_count * sizeof(usize)) / k_page_size; i++) {
        set_frame(*bitset_location / k_page_size + i);
    }
}

Optional<uintptr> MemoryManager::find_first_fit_region(usize size) {
    const usize page_count = page_round_up(size) / k_page_size;
    for (usize i = 0; i < m_boot_info->map_entry_count; i++) {
        auto &entry = m_boot_info->map[i];
        if (entry.type != MemoryType::Reserved && entry.page_count >= page_count) {
            return entry.base;
        }
    }
    return {};
}

usize &MemoryManager::frame_bitset_bucket(usize index) {
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
    const usize frame_count = page_round_up(size) / k_page_size;
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
        return reinterpret_cast<void *>(i * k_page_size);
    }
    ENSURE_NOT_REACHED("No available physical memory!");
}
