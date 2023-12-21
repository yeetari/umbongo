#include <core/event_loop.hh>
#include <core/file.hh>
#include <core/time.hh>
#include <ipc/client.hh>
#include <ipc/message_decoder.hh>
#include <ipc/server.hh>
#include <log/ipc_messages.hh>
#include <log/level.hh>
#include <ustd/array.hh>
#include <ustd/optional.hh>
#include <ustd/string.hh>
#include <ustd/string_builder.hh>
#include <ustd/string_view.hh>
#include <ustd/try.hh>
#include <ustd/types.hh>
#include <ustd/utility.hh>

namespace {

class Client final : public ipc::Client {
    core::File *m_file{nullptr};
    ustd::String m_name;

public:
    using ipc::Client::Client;
    void initialise(core::File &file, ustd::StringView name);
    void log(log::Level level, ustd::StringView message);
};

} // namespace

void Client::initialise(core::File &file, ustd::StringView name) {
    m_file = &file;
    new (&m_name) ustd::String(name);
}

void Client::log(log::Level level, ustd::StringView message) {
    constexpr ustd::Array level_strings{
        "TRACE", "DEBUG", "INFO ", "WARN ", "ERROR",
    };
    auto time = core::time() / 1000000u;
    auto line = ustd::format("[{:d5 }.{:d3}] {} [{}] {}\n", time / 1000u, time % 1000u,
                             level_strings[static_cast<size_t>(level)], m_name, message);
    EXPECT(m_file->write({line.data(), line.length()}));
}

size_t main(size_t, const char **) {
    auto file = EXPECT(core::File::open("/log", UB_OPEN_MODE_CREATE | UB_OPEN_MODE_TRUNCATE));
    core::EventLoop event_loop;
    ipc::Server<Client> server(event_loop, "/run/log"sv);
    server.set_on_message([&](Client &client, ipc::MessageDecoder &decoder) {
        using namespace log;
        switch (decoder.decode<MessageKind>()) {
        case MessageKind::Initialise: {
            auto name = decoder.decode<ustd::StringView>();
            client.initialise(file, name);
            return true;
        }
        case MessageKind::Log: {
            auto level = decoder.decode<Level>();
            auto message = decoder.decode<ustd::StringView>();
            client.log(level, message);
            return true;
        }
        default:
            return false;
        }
    });
    return event_loop.run();
}
