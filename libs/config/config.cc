#include <config/config.hh>

#include <config/ipc_messages.hh>
#include <core/event_loop.hh>
#include <ipc/client.hh>
#include <ipc/message_decoder.hh>
#include <system/system.h>
#include <ustd/assert.hh>
#include <ustd/function.hh>
#include <ustd/optional.hh>
#include <ustd/string.hh>
#include <ustd/string_view.hh>
#include <ustd/utility.hh>
#include <ustd/vector.hh>

namespace config {
namespace {

struct UpdateCallback {
    ustd::String key;
    ustd::Function<void(ustd::StringView)> callback;
};

ipc::Client *s_client = nullptr;
ustd::Vector<UpdateCallback> *s_update_callbacks = nullptr;

ipc::Client &client() {
    if (s_client == nullptr) {
        s_client = new ipc::Client;
        bool connected = s_client->connect("/run/config"sv);
        ENSURE(connected);
    }
    return *s_client;
}

} // namespace

void listen(core::EventLoop &event_loop) {
    ASSERT(s_update_callbacks == nullptr);
    s_update_callbacks = new ustd::Vector<UpdateCallback>;
    event_loop.watch(client(), UB_POLL_EVENT_READ);
    client().set_on_message([](ipc::MessageDecoder &decoder) {
        if (decoder.decode<MessageKind>() != MessageKind::NotifyChange) {
            return false;
        }
        auto key = decoder.decode<ustd::StringView>();
        auto value = decoder.decode<ustd::StringView>();
        for (auto &callback : *s_update_callbacks) {
            if (callback.key == key) {
                callback.callback(value);
            }
        }
        return true;
    });
}

void read(ustd::StringView domain) {
    client().send_message<ReadMessage>(domain);
}

ustd::Vector<ustd::String> list_all() {
    client().send_message<ListAllMessage>();
    auto response = client().wait_message<ListAllResponseMessage>();
    return ustd::move(response.list());
}

ustd::Optional<ustd::String> lookup(ustd::StringView domain, ustd::StringView key) {
    client().send_message<LookupMessage>(domain, key);
    return client().wait_message<LookupResponseMessage>().value();
}

bool update(ustd::StringView domain, ustd::StringView key, ustd::StringView value) {
    client().send_message<UpdateMessage>(domain, key, value);
    return client().wait_message<UpdateResponseMessage>().success();
}

void watch(ustd::StringView domain, ustd::StringView key, ustd::Function<void(ustd::StringView)> callback) {
    ASSERT(s_update_callbacks != nullptr);
    client().send_message<WatchMessage>(domain);
    s_update_callbacks->push(UpdateCallback{ustd::String(key), ustd::move(callback)});
}

} // namespace config
