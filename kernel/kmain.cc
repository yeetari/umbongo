#include <boot/BootInfo.hh>
#include <kernel/Config.hh>
#include <kernel/Port.hh>
#include <kernel/cpu/Processor.hh>
#include <ustd/Assert.hh>
#include <ustd/Log.hh>
#include <ustd/Types.hh>

// NOLINTNEXTLINE
usize __stack_chk_guard = 0xdeadc0de;

// NOLINTNEXTLINE
[[noreturn]] extern "C" void __stack_chk_fail() {
    ENSURE_NOT_REACHED("Stack smashing detected!");
}

void put_char(char ch) {
    if constexpr (k_kernel_qemu_debug) {
        port_write(0xe9, ch);
    }
}

void handle_interrupt(InterruptFrame *frame) {
    log("core: Received interrupt {}", frame->num);
}

extern "C" void kmain(BootInfo *boot_info) {
    if constexpr (k_kernel_stack_protector) {
        log("core: SSP initialised with guard value {:h}", __stack_chk_guard);
    }
    if constexpr (k_kernel_qemu_debug) {
        ENSURE(port_read(0xe9) == 0xe9, "KERNEL_QEMU_DEBUG config option enabled, but port e9 isn't available!");
    }

    log("core: boot_info = {}", boot_info);
    log("core: framebuffer = {:h}", boot_info->framebuffer_base);

    usize total_mem = 0;
    usize usable_mem = 0;
    for (usize i = 0; i < boot_info->map_entry_count; i++) {
        auto *entry = &boot_info->map[i];
        ENSURE(entry->base % 4096 == 0);

        const usize entry_size = entry->page_count * 4096;
        total_mem += entry_size;
        if (entry->type != MemoryType::Reserved) {
            usable_mem += entry_size;
        }
    }
    log(" mem: {}MiB/{}MiB free ({}%)", usable_mem / 1024 / 1024, total_mem / 1024 / 1024,
        (usable_mem * 100) / total_mem);

    Processor::initialise(reinterpret_cast<void *>(60_MiB), reinterpret_cast<void *>(70_MiB));
    Processor::wire_interrupt(0, &handle_interrupt);
    Processor::wire_interrupt(31, &handle_interrupt);
    Processor::wire_interrupt(255, &handle_interrupt);
    asm volatile("int $0");
    asm volatile("int $31");
    asm volatile("int $255");

    // Clear screen.
    const uint64 bytes_per_scan_line = boot_info->pixels_per_scan_line * sizeof(uint32);
    for (uint32 y = boot_info->height / 2; y < boot_info->height; y++) {
        auto *pixel = reinterpret_cast<uint32 *>(boot_info->framebuffer_base + (bytes_per_scan_line * y));
        for (uint32 x = 0; x < boot_info->width; x++) {
            *pixel++ = 255;
        }
    }
}
