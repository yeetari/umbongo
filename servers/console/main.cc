#include "EscapeParser.hh"
#include "Framebuffer.hh"
#include "Terminal.hh"

#include <core/EventLoop.hh>
#include <core/File.hh>
#include <core/Timer.hh>
#include <ipc/Client.hh>
#include <ipc/MessageDecoder.hh>
#include <ipc/Server.hh>
#include <kernel/SyscallTypes.hh>
#include <servers/console/IpcMessages.hh>
#include <ustd/Array.hh>
#include <ustd/Memory.hh>
#include <ustd/Result.hh>
#include <ustd/Span.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>

usize main(usize, const char **) {
    core::EventLoop event_loop;
    core::File stdin(static_cast<uint32>(0));
    event_loop.watch(stdin, PollEvents::Read);

    Framebuffer framebuffer("/dev/fb"sv);
    Terminal default_terminal(framebuffer);
    Terminal alternate_terminal(framebuffer);
    Terminal *terminal = &default_terminal;
    framebuffer.clear();

    core::Timer vsync_timer(event_loop, 60_Hz);
    vsync_timer.set_on_fire([&framebuffer] {
        framebuffer.swap_buffers();
    });

    EscapeParser escape_parser(default_terminal, alternate_terminal);
    stdin.set_on_read_ready([&] {
        // NOLINTNEXTLINE
        ustd::Array<char, 8_KiB> buffer;
        auto bytes_read = EXPECT(stdin.read(buffer.span()));
        terminal->clear_cursor();
        for (usize i = 0; i < bytes_read; i++) {
            char ch = buffer[i];
            if (escape_parser.parse(ch, terminal)) {
                continue;
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
    });

    ipc::Server server(event_loop, "/run/console"sv);
    server.set_on_read([terminal](ipc::Client &client, ipc::MessageDecoder &decoder) {
        using namespace console;
        switch (decoder.decode<MessageKind>()) {
        case MessageKind::GetTerminalSize:
            client.send_message<GetTerminalSizeRespone>(terminal->column_count(), terminal->row_count());
            break;
        default:
            // TODO: Disconnect client.
            break;
        }
    });
    return event_loop.run();
}
