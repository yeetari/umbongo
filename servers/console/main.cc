#include "EscapeParser.hh"
#include "Framebuffer.hh"
#include "Terminal.hh"

#include <config/Config.hh>
#include <console/IpcMessages.hh>
#include <core/EventLoop.hh>
#include <core/File.hh>
#include <core/Timer.hh>
#include <ipc/Client.hh>
#include <ipc/MessageDecoder.hh>
#include <ipc/Server.hh>
#include <kernel/SyscallTypes.hh>
#include <log/Log.hh>
#include <ustd/Array.hh>
#include <ustd/Numeric.hh>
#include <ustd/Optional.hh>
#include <ustd/Result.hh>
#include <ustd/Span.hh>
#include <ustd/StringCast.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

usize main(usize, const char **) {
    log::initialise("console-server");
    core::EventLoop event_loop;
    core::File stdin(static_cast<uint32>(0));
    event_loop.watch(stdin, kernel::PollEvents::Read);

    config::listen(event_loop);
    config::read("console-server");

    Framebuffer framebuffer("/dev/fb"sv);
    Terminal default_terminal(framebuffer);
    Terminal alternate_terminal(framebuffer);
    Terminal *terminal = &default_terminal;
    framebuffer.clear();

    core::Timer vsync_timer(event_loop, 60_Hz);
    vsync_timer.set_on_fire([&framebuffer] {
        framebuffer.swap_buffers();
    });

    config::watch("console-server", "refresh_rate", [&vsync_timer](ustd::StringView value) {
        const auto refresh_rate = ustd::max(ustd::cast<usize>(value).value_or(60), 1ul);
        vsync_timer.set_period(1_Hz / refresh_rate);
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

    ipc::Server<ipc::Client> server(event_loop, "/run/console"sv);
    server.set_on_message([terminal](ipc::Client &client, ipc::MessageDecoder &decoder) {
        using namespace console;
        switch (decoder.decode<MessageKind>()) {
        case MessageKind::GetTerminalSize:
            client.send_message<GetTerminalSizeRespone>(terminal->column_count(), terminal->row_count());
            return true;
        default:
            return false;
        }
    });
    return event_loop.run();
}
