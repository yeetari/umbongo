#include <core/EventLoop.hh>
#include <core/File.hh>
#include <core/Syscall.hh>
#include <ipc/Client.hh>
#include <ipc/MessageDecoder.hh>
#include <ipc/Server.hh>
#include <log/IpcMessages.hh>
#include <log/Level.hh>
#include <ustd/Array.hh>
#include <ustd/Format.hh>
#include <ustd/Memory.hh>
#include <ustd/Optional.hh>
#include <ustd/Result.hh>
#include <ustd/String.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

namespace {

core::File *s_file;

class Client final : public ipc::Client {
    ustd::String m_name;

public:
    Client(uint32 fd) : ipc::Client(fd) {}

    void initialise(ustd::StringView name);
    void log(log::Level level, ustd::StringView message);
};

} // namespace

void Client::initialise(ustd::StringView name) {
    new (&m_name) ustd::String(name);
}

void Client::log(log::Level level, ustd::StringView message) {
    constexpr ustd::Array level_strings{
        "TRACE", "DEBUG", "INFO ", "WARN ", "ERROR",
    };
    auto time = EXPECT(core::syscall(Syscall::gettime)) / 1000000;
    auto line = ustd::format("+{} {} [{}] {}\n", time, level_strings[static_cast<usize>(level)], m_name, message);
    EXPECT(s_file->write({line.data(), line.length()}));
}

usize main(usize, const char **) {
    auto file = EXPECT(core::File::open("/log", core::OpenMode::Create | core::OpenMode::Truncate));
    s_file = &file;
    core::EventLoop event_loop;
    ipc::Server<Client> server(event_loop, "/run/log"sv);
    server.set_on_read([&](Client &client, ipc::MessageDecoder &decoder) {
        using namespace log;
        switch (decoder.decode<MessageKind>()) {
        case MessageKind::Initialise: {
            auto name = decoder.decode<ustd::StringView>();
            client.initialise(name);
            break;
        }
        case MessageKind::Log: {
            auto level = decoder.decode<Level>();
            auto message = decoder.decode<ustd::StringView>();
            client.log(level, message);
            break;
        }
        }
    });
    return event_loop.run();
}
