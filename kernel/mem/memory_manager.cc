#include <kernel/mem/memory_manager.hh>

#include <boot/boot_info.hh>
#include <kernel/dmesg.hh>
#include <kernel/mem/address_space.hh>
#include <kernel/mem/heap.hh>
#include <kernel/mem/region.hh>
#include <kernel/mem/virtual_range.hh>
#include <kernel/mem/vm_object.hh>
#include <kernel/scoped_lock.hh>
#include <kernel/spin_lock.hh>
#include <ustd/assert.hh>
#include <ustd/numeric.hh>
#include <ustd/optional.hh>
#include <ustd/splay_tree.hh>
#include <ustd/types.hh>
#include <ustd/unique_ptr.hh>
#include <ustd/utility.hh>

namespace kernel {
namespace {

constexpr size_t k_bucket_bit_count = sizeof(size_t) * 8;
constexpr size_t k_frame_size = 4_KiB;

struct MemoryManagerData {
    size_t *frame_bitset{nullptr};
    size_t frame_count{0};
    VmObject *kernel_object{nullptr};
} s_data;
SpinLock s_lock;

ustd::Optional<uintptr_t> find_first_fit_region(BootInfo *boot_info, size_t size) {
    const size_t page_count = ustd::align_up(size, k_frame_size) / k_frame_size;
    for (size_t i = boot_info->map_entry_count; i > 0; i--) {
        auto &entry = boot_info->map[i];
        if (entry.type != MemoryType::Reserved && entry.page_count >= page_count) {
            return entry.base;
        }
    }
    return {};
}

size_t &frame_bitset_bucket(size_t index) {
    ASSERT(index < s_data.frame_count);
    return s_data.frame_bitset[(index + k_bucket_bit_count) / k_bucket_bit_count];
}

void clear_frame(size_t index) {
    frame_bitset_bucket(index) &= ~(1ul << (index % k_bucket_bit_count));
}

void set_frame(size_t index) {
    frame_bitset_bucket(index) |= 1ul << (index % k_bucket_bit_count);
}

bool is_frame_set(size_t index) {
    return (frame_bitset_bucket(index) & (1ul << (index % k_bucket_bit_count))) != 0;
}

void parse_memory_map(BootInfo *boot_info) {
    // Calculate the end of physical memory.
    uintptr_t memory_end = 0;
    for (size_t i = 0; i < boot_info->map_entry_count; i++) {
        auto &entry = boot_info->map[i];
        ENSURE(entry.base % k_frame_size == 0);

        const uintptr_t entry_end = entry.base + (entry.page_count * k_frame_size);
        if (entry_end > memory_end) {
            memory_end = entry_end;
        }
    }

    // Calculate total frame count.
    ASSERT(memory_end % k_frame_size == 0);
    s_data.frame_count = memory_end / k_frame_size;

    // Find some free memory for the physical frame bitset. We do this by finding the first fit entry in the memory map.
    const size_t bucket_count = s_data.frame_count / k_bucket_bit_count + 1;
    const auto bitset_location = find_first_fit_region(boot_info, bucket_count * sizeof(size_t));
    ENSURE(bitset_location, "Failed to allocate memory for physical frame bitset!");

    // Construct the frame bitset and initially mark all frames as reserved.
    s_data.frame_bitset = new (reinterpret_cast<void *>(*bitset_location)) size_t[bucket_count];
    __builtin_memset(s_data.frame_bitset, 0xff, bucket_count * sizeof(size_t));

    // Mark available frames as usable. Reclaimable memory will be reclaimed later, after the initial kernel stack is no
    // longer in use.
    for (size_t i = 0; i < boot_info->map_entry_count; i++) {
        auto &entry = boot_info->map[i];
        if (entry.type != MemoryType::Available) {
            continue;
        }
        for (size_t j = 0; j < entry.page_count; j++) {
            clear_frame(entry.base / k_frame_size + j);
        }
    }

    // Remark the memory for the frame bitset itself as reserved.
    set_frame(0);
    for (size_t i = 0; i < ustd::align_up(bucket_count * sizeof(size_t), k_frame_size) / k_frame_size; i++) {
        set_frame(*bitset_location / k_frame_size + i);
    }

    // Print some memory info.
    size_t free_bytes = 0;
    size_t total_bytes = 0;
    for (size_t i = 0; i < s_data.frame_count; i++) {
        free_bytes += !is_frame_set(i) ? k_frame_size : 0;
        total_bytes += k_frame_size;
    }
    dmesg(" mem: {}MiB/{}MiB free ({}%)", free_bytes / 1_MiB, total_bytes / 1_MiB, (free_bytes * 100) / total_bytes);
}

} // namespace

void MemoryManager::initialise(BootInfo *boot_info) {
    parse_memory_map(boot_info);

    // Reserve 0x8000 for the SMP trampoline.
    // TODO: This is x86 specific.
    ENSURE(is_frame_free(0x8000));
    set_frame(0x8000 / k_frame_size);

    Heap::initialise();

    // Create the identity map VmObject.
    auto kernel_object = VmObject::create_physical(0, 512_GiB);
    s_data.kernel_object = kernel_object.disown();
}

void MemoryManager::reclaim(BootInfo *boot_info) {
    size_t total_reclaimed = 0;
    for (size_t i = 0; i < boot_info->map_entry_count; i++) {
        auto &entry = boot_info->map[i];
        if (entry.base == 0 || entry.type != MemoryType::Reclaimable) {
            continue;
        }
        for (size_t j = 0; j < entry.page_count; j++) {
            clear_frame(entry.base / k_frame_size + j);
            total_reclaimed += k_frame_size;
        }
    }
    if (total_reclaimed >= 1_MiB) {
        dmesg(" mem: Reclaimed {}MiB of memory", total_reclaimed / 1_MiB);
    } else if (total_reclaimed >= 1_KiB) {
        dmesg(" mem: Reclaimed {}KiB of memory", total_reclaimed / 1_KiB);
    } else {
        dmesg(" mem: Reclaimed {}B of memory", total_reclaimed);
    }
}

uintptr_t MemoryManager::alloc_frame() {
    ScopedLock locker(s_lock);
    for (size_t i = 0; i < s_data.frame_count; i++) {
        if (!is_frame_set(i)) {
            set_frame(i);
            return i * k_frame_size;
        }
    }
    ENSURE_NOT_REACHED("No available physical memory!");
}

void MemoryManager::free_frame(uintptr_t frame) {
    ScopedLock locker(s_lock);
    ASSERT(frame % k_frame_size == 0);
    const auto index = frame / k_frame_size;
    ASSERT(is_frame_set(index));
    clear_frame(index);
}

bool MemoryManager::is_frame_free(uintptr_t frame) {
    ASSERT(frame % k_frame_size == 0);
    return !is_frame_set(frame / k_frame_size);
}

void *MemoryManager::alloc_contiguous(size_t size) {
    ScopedLock locker(s_lock);
    const size_t frame_count = ustd::align_up(size, k_frame_size) / k_frame_size;
    for (size_t i = 0; i < s_data.frame_count; i += frame_count) {
        bool found_space = true;
        for (size_t j = i; j < i + frame_count; j++) {
            if (is_frame_set(j)) {
                found_space = false;
                break;
            }
        }

        if (!found_space) {
            continue;
        }

        for (size_t j = i; j < i + frame_count; j++) {
            set_frame(j);
        }
        return reinterpret_cast<void *>(i * k_frame_size);
    }
    ENSURE_NOT_REACHED("No available physical memory!");
}

void MemoryManager::free_contiguous(void *ptr, size_t size) {
    ScopedLock locker(s_lock);
    const auto first_frame = reinterpret_cast<uintptr_t>(ptr);
    ASSERT(first_frame % k_frame_size == 0);
    const auto first_frame_index = first_frame / k_frame_size;
    const size_t frame_count = ustd::align_up(size, k_frame_size) / k_frame_size;
    for (size_t i = first_frame_index; i < first_frame_index + frame_count; i++) {
        ASSERT(is_frame_set(i));
        clear_frame(i);
    }
}

VmObject *MemoryManager::kernel_object() {
    return s_data.kernel_object;
}

} // namespace kernel
