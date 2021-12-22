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
    ustd::Function<void(ClientType &, MessageDecoder &)> m_on_read;

public:
    Server(core::EventLoop &event_loop, ustd::StringView path);
    Server(const Server &) = delete;
    Server(Server &&) = delete;
    ~Server() override;

    Server &operator=(const Server &) = delete;
    Server &operator=(Server &&) = delete;

    void set_on_read(ustd::Function<void(ClientType &, MessageDecoder &)> on_read) { m_on_read = ustd::move(on_read); }
    uint32 fd() const override { return *m_fd; }
};

template <typename ClientType>
Server<ClientType>::Server(core::EventLoop &event_loop, ustd::StringView path) : m_event_loop(event_loop) {
    m_fd.emplace(EXPECT(core::syscall<uint32>(Syscall::create_server_socket, 4)));
    EXPECT(core::syscall(Syscall::bind, *m_fd, path.data()));
    event_loop.watch(*this, kernel::PollEvents::Accept);
    set_on_read_ready([this] {
        uint32 client_fd = EXPECT(core::syscall<uint32>(Syscall::accept, *m_fd));
        auto *client = m_clients.emplace(new ClientType(client_fd)).obj();
        m_event_loop.watch(*client, kernel::PollEvents::Read);
        client->set_on_read_ready([this, client] {
            // NOLINTNEXTLINE
            ustd::Array<uint8, 8_KiB> buffer;
            usize bytes_read = client->wait_message(buffer.span());
            if (bytes_read == 0) {
                m_event_loop.unwatch(*client);
                for (uint32 i = 0; i < m_clients.size(); i++) {
                    if (m_clients[i].obj() == client) {
                        m_clients.remove(i);
                        return;
                    }
                }
                ENSURE_NOT_REACHED();
            }
            ASSERT(m_on_read);
            for (usize position = 0u; position < bytes_read;) {
                MessageDecoder decoder({buffer.data() + position, bytes_read});
                m_on_read(*client, decoder);
                position += decoder.bytes_decoded();
            }
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

} // namespace ipc
