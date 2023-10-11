#include <log/Log.hh>

#include <core/Syscall.hh>
#include <ipc/Client.hh>
#include <log/IpcMessages.hh>
#include <ustd/Assert.hh>
#include <ustd/StringView.hh>

namespace log {
namespace {

ipc::Client *s_client = nullptr;

} // namespace

enum class Level;

void initialise(ustd::StringView name) {
    ASSERT(s_client == nullptr);
    s_client = new ipc::Client;
    if (s_client->connect("/run/log"sv)) {
        s_client->send_message<InitialiseMessage>(name);
    }
}

void message(Level level, ustd::StringView message) {
    static_cast<void>(core::syscall(Syscall::debug_line, message.data()));
    if (s_client != nullptr && s_client->connected()) {
        s_client->send_message<LogMessage>(level, message);
        return;
    }
    const char newline = '\n';
    static_cast<void>(core::syscall(Syscall::write, 1, message.data(), message.length()));
    static_cast<void>(core::syscall(Syscall::write, 1, &newline, 1));
}

} // namespace log
