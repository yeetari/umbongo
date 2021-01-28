#include <boot/BootInfo.hh>
#include <ustd/Types.hh>

extern "C" void kmain(BootInfo *boot_info) {
    // Write to QEMU debug port.
    const char *msg = "Hello, world!\n\0";
    for (; *msg != '\0'; msg++) {
        asm volatile("out %0, %1" : : "a"(*msg), "Nd"(0xE9));
    }

    // Clear screen.
    const uint64 bytes_per_scan_line = boot_info->pixels_per_scan_line * sizeof(uint32);
    for (uint32 y = 0; y < boot_info->height; y++) {
        auto *pixel = reinterpret_cast<uint32 *>(boot_info->framebuffer_base + (bytes_per_scan_line * y));
        for (uint32 x = 0; x < boot_info->width; x++) {
            *pixel++ = 0;
        }
    }
}
