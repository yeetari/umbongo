#include "Framebuffer.hh"
#include "Terminal.hh"

#include <kernel/Syscall.hh>
#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/Log.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

usize main(usize, const char **) {
    Framebuffer framebuffer("/dev/fb"sv);
    Terminal default_terminal(framebuffer);
    Terminal alternate_terminal(framebuffer);
    Terminal *terminal = &default_terminal;
    framebuffer.clear();

    bool in_escape = false;
    uint32 escape_param = 0;
    Vector<uint32> escape_params;
    while (true) {
        // NOLINTNEXTLINE
        Array<char, 8192> buf;
        auto nread = Syscall::invoke(Syscall::read, 0, buf.data(), buf.size());
        if (nread < 0) {
            ENSURE_NOT_REACHED();
        } else if (nread == 0) {
            continue;
        }

        terminal->clear_cursor();
        for (usize i = 0; i < static_cast<usize>(nread); i++) {
            char ch = buf[i];
            if (ch == '\x1b') {
                in_escape = true;
                escape_params.clear();
                continue;
            }
            if (in_escape && ch == '[') {
                continue;
            }
            if (in_escape && ch == '?') {
                continue;
            }
            if (in_escape) {
                if (ch - '0' < 10) {
                    escape_param *= 10;
                    escape_param += (static_cast<unsigned char>(ch) - '0');
                    continue;
                }
                if (ch == ';') {
                    escape_params.push(escape_param);
                    escape_param = 0;
                    continue;
                }
                escape_params.push(escape_param);
                escape_param = 0;
                in_escape = false;
                switch (ch) {
                case 'D':
                    terminal->move_left();
                    continue;
                case 'C':
                    terminal->move_right();
                    continue;
                case 'H':
                    if (escape_params.size() != 2) {
                        dbgln("CUP doesn't have row and column");
                        break;
                    }
                    terminal->set_cursor(escape_params[1], escape_params[0]);
                    continue;
                case 'J':
                    terminal->clear();
                    continue;
                case 'h':
                    if (escape_params.size() != 1) {
                        dbgln("Private mode enable doesn't have code");
                        break;
                    }
                    if (escape_params[0] != 1049) {
                        dbgln("Unknown private code {}", escape_params[0]);
                        break;
                    }
                    terminal = &alternate_terminal;
                    terminal->clear();
                    continue;
                case 'l':
                    if (escape_params.size() != 1) {
                        dbgln("Private mode disable doesn't have code");
                        break;
                    }
                    if (escape_params[0] != 1049) {
                        dbgln("Unknown private code {}", escape_params[0]);
                        break;
                    }
                    terminal = &default_terminal;
                    terminal->set_dirty();
                    continue;
                case 'm':
                    if (escape_params.empty()) {
                        escape_params.push(0);
                    }
                    switch (escape_params[0]) {
                    case 38: // Set foreground colour
                        if (escape_params.size() < 2) {
                            dbgln("SGR has no colour type");
                            break;
                        }
                        if (escape_params[1] != 2) {
                            dbgln("Unknown colour type {}", escape_params[1]);
                            break;
                        }
                        if (escape_params.size() != 5) {
                            dbgln("Incorrect number of parameters for 24-bit colour set");
                            break;
                        }
                        terminal->set_colour(escape_params[2], escape_params[3], escape_params[4]);
                        break;
                    default:
                        dbgln("Unknown SGR code {}", escape_params[0]);
                        break;
                    }
                    continue;
                default:
                    dbg("Unknown CSI code {:c}", ch);
                    continue;
                }
            }

            switch (ch) {
            case '\b':
                terminal->backspace();
                break;
            case '\n':
                terminal->newline();
                break;
            default:
                terminal->put_char(ch);
                break;
            }
        }
        terminal->render();
    }
}
