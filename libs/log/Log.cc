#include <log/Log.hh>

#include <ipc/Client.hh>
#include <log/IpcMessages.hh>
#include <log/Level.hh>
#include <ustd/Assert.hh>
#include <ustd/StringView.hh>

namespace log {
namespace {

ipc::Client *s_client = nullptr;

} // namespace

void initialise(ustd::StringView name) {
    ASSERT(s_client == nullptr);
    s_client = new ipc::Client;
    s_client->connect("/run/log"sv);
    s_client->send_message<InitialiseMessage>(name);
}

void message(Level level, ustd::StringView message) {
    ASSERT(s_client != nullptr);
    s_client->send_message<LogMessage>(level, message);
}

} // namespace log
