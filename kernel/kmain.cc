#include <boot/BootInfo.hh>
#include <kernel/Port.hh>
#include <ustd/Assert.hh>
#include <ustd/Log.hh>
#include <ustd/Types.hh>

[[noreturn]] void assertion_failed(const char *file, unsigned int line, const char *expr, const char *msg) {
    log("Assertion '{}' failed at {}:{}", expr, file, line);
    log("=> {}", msg);
    while (true) {
        asm volatile("cli");
        asm volatile("hlt");
    }
}

void put_char(char ch) {
    port_write(0xE9, ch);
}

extern "C" void kmain(BootInfo *boot_info) {
    if (port_read(0xE9) == 0xE9) {
        log("Hello, world!");
        log("boot_info = {}", boot_info);
        log("framebuffer = {:h}", boot_info->framebuffer_base);
    }

    // Clear screen.
    const uint64 bytes_per_scan_line = boot_info->pixels_per_scan_line * sizeof(uint32);
    for (uint32 y = boot_info->height / 2; y < boot_info->height; y++) {
        auto *pixel = reinterpret_cast<uint32 *>(boot_info->framebuffer_base + (bytes_per_scan_line * y));
        for (uint32 x = 0; x < boot_info->width; x++) {
            *pixel++ = 255;
        }
    }
}
