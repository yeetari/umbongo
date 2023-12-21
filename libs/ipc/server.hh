#pragma once

#include <core/event_loop.hh>
#include <core/watchable.hh>
#include <system/syscall.hh>
#include <system/system.h>
#include <ustd/assert.hh>
#include <ustd/function.hh>
#include <ustd/optional.hh>
#include <ustd/string_view.hh>
#include <ustd/try.hh>
#include <ustd/types.hh>
#include <ustd/unique_ptr.hh> // IWYU pragma: keep
#include <ustd/utility.hh>
#include <ustd/vector.hh>

namespace ipc {

class MessageDecoder;

template <typename ClientType>
class Server : public core::Watchable {
    core::EventLoop &m_event_loop;
    ustd::Optional<uint32_t> m_fd;
    ustd::Vector<ustd::UniquePtr<ClientType>> m_clients;
    ustd::Function<bool(ClientType &, MessageDecoder &)> m_on_message;

    void disconnect(ClientType *client);

public:
    Server(core::EventLoop &event_loop, ustd::StringView path);
    Server(const Server &) = delete;
    Server(Server &&) = delete;
    ~Server() override;

    Server &operator=(const Server &) = delete;
    Server &operator=(Server &&) = delete;

    void set_on_message(ustd::Function<bool(ClientType &, MessageDecoder &)> on_message) {
        m_on_message = ustd::move(on_message);
    }
    uint32_t fd() const override { return *m_fd; }
    const ustd::Vector<ustd::UniquePtr<ClientType>> &clients() const { return m_clients; }
};

template <typename ClientType>
Server<ClientType>::Server(core::EventLoop &event_loop, ustd::StringView path) : m_event_loop(event_loop) {
    m_fd.emplace(EXPECT(system::syscall<uint32_t>(UB_SYS_create_server_socket, 4)));
    EXPECT(system::syscall(UB_SYS_bind, *m_fd, path.data()));
    event_loop.watch(*this, UB_POLL_EVENT_ACCEPT);
    set_on_read_ready([this] {
        uint32_t client_fd = EXPECT(system::syscall<uint32_t>(UB_SYS_accept, *m_fd));
        auto *client = m_clients.emplace(new ClientType(client_fd)).ptr();
        m_event_loop.watch(*client, UB_POLL_EVENT_READ);
        client->set_on_disconnect([this, client] {
            disconnect(client);
        });
        client->set_on_message([this, client](ipc::MessageDecoder &decoder) {
            return m_on_message(*client, decoder);
        });
    });
}

template <typename ClientType>
Server<ClientType>::~Server() {
    m_event_loop.unwatch(*this);
    for (auto &client : m_clients) {
        m_event_loop.unwatch(*client);
    }
}

template <typename ClientType>
void Server<ClientType>::disconnect(ClientType *client) {
    m_event_loop.unwatch(*client);
    for (uint32_t i = 0; i < m_clients.size(); i++) {
        if (m_clients[i].ptr() == client) {
            m_clients.remove(i);
            return;
        }
    }
    ENSURE_NOT_REACHED();
}

} // namespace ipc
