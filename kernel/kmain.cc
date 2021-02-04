#include <boot/BootInfo.hh>
#include <kernel/Config.hh>
#include <kernel/Port.hh>
#include <ustd/Assert.hh>
#include <ustd/Log.hh>
#include <ustd/Types.hh>

// NOLINTNEXTLINE
usize __stack_chk_guard = 0xDEADC0DE;

// NOLINTNEXTLINE
[[noreturn]] extern "C" void __stack_chk_fail() {
    ENSURE_NOT_REACHED("Stack smashing detected!");
}

void put_char(char ch) {
    if constexpr (KERNEL_QEMU_DEBUG) {
        port_write(0xE9, ch);
    }
}

extern "C" void kmain(BootInfo *boot_info) {
    if constexpr (KERNEL_STACK_PROTECTOR) {
        log("SSP initialised with guard value {:h}", __stack_chk_guard);
    }
    if constexpr (KERNEL_QEMU_DEBUG) {
        ENSURE(port_read(0xE9) == 0xE9, "KERNEL_QEMU_DEBUG config option enabled, but port E9 isn't available!");
    }

    log("Hello, world!");
    log("boot_info = {}", boot_info);
    log("framebuffer = {:h}", boot_info->framebuffer_base);

    // Clear screen.
    const uint64 bytes_per_scan_line = boot_info->pixels_per_scan_line * sizeof(uint32);
    for (uint32 y = boot_info->height / 2; y < boot_info->height; y++) {
        auto *pixel = reinterpret_cast<uint32 *>(boot_info->framebuffer_base + (bytes_per_scan_line * y));
        for (uint32 x = 0; x < boot_info->width; x++) {
            *pixel++ = 255;
        }
    }
}
