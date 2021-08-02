#include <core/File.hh>
#include <kernel/Font.hh>
#include <kernel/Syscall.hh>
#include <kernel/SyscallTypes.hh>
#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>

usize main(usize, const char **) {
    core::File framebuffer("/dev/fb");
    ENSURE(framebuffer);

    auto *fb = Syscall::invoke<uint32 *>(Syscall::mmap, framebuffer.fd());
    Syscall::invoke(Syscall::ioctl, framebuffer.fd(), IoctlRequest::FramebufferClear);

    FramebufferInfo fb_info{};
    Syscall::invoke(Syscall::ioctl, framebuffer.fd(), IoctlRequest::FramebufferGetInfo, &fb_info);

    uint32 current_x = 0;
    uint32 current_y = 0;
    while (true) {
        // NOLINTNEXTLINE
        Array<char, 8192> buf;
        // TODO: This is non-blocking.
        auto nread = Syscall::invoke(Syscall::read, 0, buf.data(), buf.size());
        if (nread < 0) {
            ENSURE_NOT_REACHED();
        } else if (nread == 0) {
            continue;
        }

        for (usize i = 0; i < static_cast<usize>(nread); i++) {
            char ch = buf[i];
            if (ch == '\n') {
                current_x = 0;
                current_y += g_font.line_height();
                continue;
            }
            if (ch == '\b') {
                if (current_x < g_font.advance()) {
                    continue;
                }
                current_x -= g_font.advance();
                for (uint32 y = 0; y < g_font.line_height(); y++) {
                    for (uint32 x = 0; x < g_font.advance(); x++) {
                        fb[(current_y + y) * fb_info.height + (current_x + x)] = 0;
                    }
                }
                continue;
            }
            const auto *glyph = g_font.glyph(ch);
            for (uint32 y1 = 0; y1 < glyph->height; y1++) {
                for (uint32 x1 = 0; x1 < glyph->width; x1++) {
                    const uint32 colour = glyph->bitmap[y1 * glyph->width + x1];
                    const auto x2 = static_cast<uint32>(static_cast<int32>(x1 + current_x) + glyph->left);
                    const auto y2 =
                        static_cast<uint32>(static_cast<int32>(y1 + current_y + g_font.ascender()) - glyph->top);
                    fb[y2 * fb_info.height + x2] = colour | (colour << 8u) | (colour << 16u);
                }
            }
            current_x += g_font.advance();
        }
    }
}
