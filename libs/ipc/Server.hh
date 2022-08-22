#pragma once

#include <core/EventLoop.hh>
#include <core/Syscall.hh>
#include <core/Watchable.hh>
#include <ipc/Client.hh>
#include <ipc/MessageDecoder.hh>
#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/Function.hh>
#include <ustd/Optional.hh>
#include <ustd/Result.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>
#include <ustd/Utility.hh>
#include <ustd/Vector.hh>

namespace ipc {

template <typename ClientType>
class Server : public core::Watchable {
    core::EventLoop &m_event_loop;
    ustd::Optional<uint32> m_fd;
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
    uint32 fd() const override { return *m_fd; }
    const ustd::Vector<ustd::UniquePtr<ClientType>> &clients() const { return m_clients; }
};

template <typename ClientType>
Server<ClientType>::Server(core::EventLoop &event_loop, ustd::StringView path) : m_event_loop(event_loop) {
    m_fd.emplace(EXPECT(core::syscall<uint32>(Syscall::create_server_socket, 4)));
    EXPECT(core::syscall(Syscall::bind, *m_fd, path.data()));
    event_loop.watch(*this, kernel::PollEvents::Accept);
    set_on_read_ready([this] {
        uint32 client_fd = EXPECT(core::syscall<uint32>(Syscall::accept, *m_fd));
        auto *client = m_clients.emplace(new ClientType(client_fd)).ptr();
        m_event_loop.watch(*client, kernel::PollEvents::Read);
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
    for (uint32 i = 0; i < m_clients.size(); i++) {
        if (m_clients[i].ptr() == client) {
            m_clients.remove(i);
            return;
        }
    }
    ENSURE_NOT_REACHED();
}

} // namespace ipc
