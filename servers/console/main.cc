#include "Framebuffer.hh"
#include "Terminal.hh"

#include <kernel/Syscall.hh>
#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>

usize main(usize, const char **) {
    Framebuffer framebuffer("/dev/fb"sv);
    Terminal terminal(framebuffer);
    framebuffer.clear();

    bool in_escape = false;
    while (true) {
        // NOLINTNEXTLINE
        Array<char, 8192> buf;
        auto nread = Syscall::invoke(Syscall::read, 0, buf.data(), buf.size());
        if (nread < 0) {
            ENSURE_NOT_REACHED();
        } else if (nread == 0) {
            continue;
        }

        terminal.clear_cursor();
        for (usize i = 0; i < static_cast<usize>(nread); i++) {
            char ch = buf[i];
            if (ch == '\x1b') {
                in_escape = true;
                continue;
            }
            if (in_escape && ch == '[') {
                continue;
            }
            if (in_escape) {
                in_escape = false;
                switch (ch) {
                case 'D':
                    terminal.move_left();
                    continue;
                case 'C':
                    terminal.move_right();
                    continue;
                case 'J':
                    terminal.clear();
                    continue;
                default:
                    ENSURE_NOT_REACHED();
                }
            }

            switch (ch) {
            case '\b':
                terminal.backspace();
                break;
            case '\n':
                terminal.newline();
                break;
            default:
                terminal.put_char(ch);
                break;
            }
        }
        terminal.render();
    }
}
