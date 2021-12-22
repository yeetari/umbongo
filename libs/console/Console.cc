#include <console/Console.hh>

#include <console/IpcMessages.hh>
#include <ipc/Client.hh>
#include <ustd/Assert.hh>
#include <ustd/StringView.hh>

namespace console {
namespace {

ipc::Client *s_client = nullptr;

ipc::Client *client() {
    if (s_client == nullptr) {
        s_client = new ipc::Client;
        bool connected = s_client->connect("/run/console"sv);
        ENSURE(connected);
    }
    return s_client;
}

} // namespace

TerminalSize terminal_size() {
    client()->send_message<GetTerminalSize>();
    auto response = client()->wait_message<GetTerminalSizeRespone>();
    return {response.column_count(), response.row_count()};
}

} // namespace console
