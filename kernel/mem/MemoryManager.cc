#include <kernel/mem/MemoryManager.hh>

#include <boot/BootInfo.hh>
#include <kernel/ScopedLock.hh> // IWYU pragma: keep
#include <kernel/SpinLock.hh>
#include <kernel/cpu/InterruptDisabler.hh>
#include <kernel/cpu/Processor.hh>
#include <kernel/mem/Region.hh>
#include <kernel/mem/VirtSpace.hh>
#include <ustd/Assert.hh>
#include <ustd/Log.hh>
#include <ustd/Memory.hh>
#include <ustd/Optional.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>

namespace {

constexpr usize k_bucket_bit_count = sizeof(usize) * 8;
constexpr usize k_frame_size = 4_KiB;

struct MemoryManagerData {
    usize *frame_bitset{nullptr};
    usize frame_count{0};
    VirtSpace *kernel_space{nullptr};
} s_data;
SpinLock s_lock;

Optional<uintptr> find_first_fit_region(BootInfo *boot_info, usize size) {
    const usize page_count = round_up(size, k_frame_size) / k_frame_size;
    for (usize i = boot_info->map_entry_count; i > 0; i--) {
        auto &entry = boot_info->map[i];
        if (entry.type != MemoryType::Reserved && entry.page_count >= page_count) {
            return entry.base;
        }
    }
    return {};
}

usize &frame_bitset_bucket(usize index) {
    ASSERT(index < s_data.frame_count);
    return s_data.frame_bitset[(index + k_bucket_bit_count) / k_bucket_bit_count];
}

void clear_frame(usize index) {
    frame_bitset_bucket(index) &= ~(1ul << (index % k_bucket_bit_count));
}

void set_frame(usize index) {
    frame_bitset_bucket(index) |= 1ul << (index % k_bucket_bit_count);
}

bool is_frame_set(usize index) {
    return (frame_bitset_bucket(index) & (1ul << (index % k_bucket_bit_count))) != 0;
}

void parse_memory_map(BootInfo *boot_info) {
    // Calculate the end of physical memory.
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
    s_data.frame_count = memory_end / k_frame_size;

    // Find some free memory for the physical frame bitset. We do this by finding the first fit entry in the memory map.
    const usize bucket_count = s_data.frame_count / k_bucket_bit_count + 1;
    const auto bitset_location = find_first_fit_region(boot_info, bucket_count * sizeof(usize));
    ENSURE(bitset_location, "Failed to allocate memory for physical frame bitset!");

    // Construct the frame bitset and initially mark all frames as reserved.
    s_data.frame_bitset = new (reinterpret_cast<void *>(*bitset_location)) usize[bucket_count];
    memset(s_data.frame_bitset, 0xff, bucket_count * sizeof(usize));

    // Mark available frames as usable. Reclaimable memory will be reclaimed later, after the initial kernel stack is no
    // longer in use.
    for (usize i = 0; i < boot_info->map_entry_count; i++) {
        auto &entry = boot_info->map[i];
        if (entry.type != MemoryType::Available) {
            continue;
        }
        for (usize j = 0; j < entry.page_count; j++) {
            clear_frame(entry.base / k_frame_size + j);
        }
    }

    // Remark the memory for the frame bitset itself as reserved.
    set_frame(0);
    for (usize i = 0; i < round_up(bucket_count * sizeof(usize), k_frame_size) / k_frame_size; i++) {
        set_frame(*bitset_location / k_frame_size + i);
    }

    // Print some memory info.
    usize free_bytes = 0;
    usize total_bytes = 0;
    for (usize i = 0; i < s_data.frame_count; i++) {
        free_bytes += !is_frame_set(i) ? k_frame_size : 0;
        total_bytes += k_frame_size;
    }
    dbgln(" mem: {}MiB/{}MiB free ({}%)", free_bytes / 1_MiB, total_bytes / 1_MiB, (free_bytes * 100) / total_bytes);
}

} // namespace

void MemoryManager::initialise(BootInfo *boot_info) {
    parse_memory_map(boot_info);
    ENSURE(is_frame_free(0x8000));
    set_frame(0x8000 / k_frame_size);

    // Create the kernel virtual space and identity map physical memory up to 512GiB. Using 1GiB pages means this only
    // takes 4KiB of page structures to do.
    s_data.kernel_space = new VirtSpace;
    s_data.kernel_space->create_region(0, 512_GiB, RegionAccess::Writable | RegionAccess::Executable, 0);

    // Leak a ref for the kernel space to ensure it never gets deleted by a kernel process being destroyed.
    s_data.kernel_space->leak_ref();
}

void MemoryManager::reclaim(BootInfo *boot_info) {
    InterruptDisabler disabler;
    usize total_reclaimed = 0;
    for (usize i = 0; i < boot_info->map_entry_count; i++) {
        auto &entry = boot_info->map[i];
        if (entry.base == 0 || entry.type != MemoryType::Reclaimable) {
            continue;
        }
        for (usize j = 0; j < entry.page_count; j++) {
            clear_frame(entry.base / k_frame_size + j);
            total_reclaimed += k_frame_size;
        }
    }
    if (total_reclaimed >= 1_MiB) {
        dbgln(" mem: Reclaimed {}MiB of memory", total_reclaimed / 1_MiB);
    } else if (total_reclaimed >= 1_KiB) {
        dbgln(" mem: Reclaimed {}KiB of memory", total_reclaimed / 1_KiB);
    } else {
        dbgln(" mem: Reclaimed {}B of memory", total_reclaimed);
    }
}

void MemoryManager::switch_space(VirtSpace &virt_space) {
    Processor::set_current_space(&virt_space);
    Processor::write_cr3(virt_space.m_pml4.obj());
}

uintptr MemoryManager::alloc_frame() {
    ScopedLock locker(s_lock);
    InterruptDisabler disabler;
    for (usize i = 0; i < s_data.frame_count; i++) {
        if (!is_frame_set(i)) {
            set_frame(i);
            return i * k_frame_size;
        }
    }
    ENSURE_NOT_REACHED("No available physical memory!");
}

void MemoryManager::free_frame(uintptr frame) {
    ScopedLock locker(s_lock);
    InterruptDisabler disabler;
    ASSERT(frame % k_frame_size == 0);
    const auto index = frame / k_frame_size;
    ASSERT(is_frame_set(index));
    clear_frame(index);
}

bool MemoryManager::is_frame_free(uintptr frame) {
    ASSERT(frame % k_frame_size == 0);
    return !is_frame_set(frame / k_frame_size);
}

void *MemoryManager::alloc_contiguous(usize size) {
    ScopedLock locker(s_lock);
    InterruptDisabler disabler;
    const usize frame_count = round_up(size, k_frame_size) / k_frame_size;
    for (usize i = 0; i < s_data.frame_count; i += frame_count) {
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

void MemoryManager::free_contiguous(void *ptr, usize size) {
    ScopedLock locker(s_lock);
    InterruptDisabler disabler;
    const auto first_frame = reinterpret_cast<uintptr>(ptr);
    ASSERT(first_frame % k_frame_size == 0);
    const auto first_frame_index = first_frame / k_frame_size;
    const usize frame_count = round_up(size, k_frame_size) / k_frame_size;
    for (usize i = first_frame_index; i < first_frame_index + frame_count; i++) {
        ASSERT(is_frame_set(i));
        clear_frame(i);
    }
}

VirtSpace *MemoryManager::current_space() {
    return Processor::current_space();
}

VirtSpace *MemoryManager::kernel_space() {
    return s_data.kernel_space;
}
