#include <kernel/Console.hh>

#include <boot/BootInfo.hh>
#include <kernel/Font.hh>
#include <ustd/Assert.hh>
#include <ustd/Types.hh>

namespace {

uint64 s_bytes_per_scan_line;
uintptr s_framebuffer_base;
uint32 s_framebuffer_width;
uint32 s_framebuffer_height;

uint32 s_current_x = 0;
uint32 s_current_y = 0;

void set_absolute_pixel(uint32 x, uint32 y, uint32 colour) {
    if (x >= s_framebuffer_width || y >= s_framebuffer_height) {
        return;
    }
    *(reinterpret_cast<uint32 *>(s_framebuffer_base + y * s_bytes_per_scan_line) + x) = colour;
}

} // namespace

void Console::initialise(BootInfo *boot_info) {
    s_bytes_per_scan_line = boot_info->pixels_per_scan_line * sizeof(uint32);
    s_framebuffer_base = boot_info->framebuffer_base;
    s_framebuffer_width = boot_info->width;
    s_framebuffer_height = boot_info->height;

    // Clear screen.
    for (uint32 y = 0; y < s_framebuffer_height; y++) {
        for (uint32 x = 0; x < s_framebuffer_width; x++) {
            set_absolute_pixel(x, y, 0);
        }
    }
}

void Console::put_char(char ch) {
    if (ch == '\n') {
        s_current_x = 0;
        s_current_y += g_font.line_height();
        return;
    }
    const auto *glyph = g_font.glyph(ch);
    for (uint32 y1 = 0; y1 < glyph->height; y1++) {
        for (uint32 x1 = 0; x1 < glyph->width; x1++) {
            const uint32 colour = glyph->bitmap[y1 * glyph->width + x1];
            const uint32 x2 = x1 + s_current_x + glyph->left;
            const uint32 y2 = y1 + s_current_y + g_font.ascender() - glyph->top;
            set_absolute_pixel(x2, y2, colour | (colour << 8u) | (colour << 16u));
        }
    }
    s_current_x += g_font.advance();
}
