#include <kernel/Console.hh>

#include <boot/BootInfo.hh>
#include <kernel/Font.hh>
#include <ustd/Types.hh>

namespace kernel {
namespace {

uint64_t s_bytes_per_scan_line;
uintptr_t s_framebuffer_base;
uint32_t s_framebuffer_width;
uint32_t s_framebuffer_height;

uint32_t s_current_x = 0;
uint32_t s_current_y = 0;

void set_absolute_pixel(uint32_t x, uint32_t y, uint32_t colour) {
    if (x >= s_framebuffer_width || y >= s_framebuffer_height) {
        return;
    }
    *(reinterpret_cast<uint32_t *>(s_framebuffer_base + y * s_bytes_per_scan_line) + x) = colour;
}

} // namespace

void Console::initialise(BootInfo *boot_info) {
    s_bytes_per_scan_line = boot_info->pixels_per_scan_line * sizeof(uint32_t);
    s_framebuffer_base = boot_info->framebuffer_base;
    s_framebuffer_width = boot_info->width;
    s_framebuffer_height = boot_info->height;

    // Clear screen.
    for (uint32_t y = 0; y < s_framebuffer_height; y++) {
        for (uint32_t x = 0; x < s_framebuffer_width; x++) {
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
    for (uint32_t y1 = 0; y1 < glyph->height; y1++) {
        for (uint32_t x1 = 0; x1 < glyph->width; x1++) {
            const uint32_t colour = glyph->bitmap[y1 * glyph->width + x1];
            const auto x2 = static_cast<uint32_t>(static_cast<int32_t>(x1 + s_current_x) + glyph->left);
            const auto y2 =
                static_cast<uint32_t>(static_cast<int32_t>(y1 + s_current_y + g_font.ascender()) - glyph->top);
            set_absolute_pixel(x2, y2, colour | (colour << 8u) | (colour << 16u));
        }
    }
    s_current_x += g_font.advance();
}

} // namespace kernel
