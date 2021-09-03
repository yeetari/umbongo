#include <ipc/Server.hh>

#include <core/EventLoop.hh>
#include <ipc/Client.hh>
#include <ipc/MessageDecoder.hh>
#include <kernel/Syscall.hh>
#include <kernel/SyscallTypes.hh>
#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/Function.hh>
#include <ustd/Memory.hh>
#include <ustd/Optional.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>
#include <ustd/Vector.hh>

namespace ipc {

Server::Server(core::EventLoop &event_loop, StringView path) : m_event_loop(event_loop) {
    if (auto rc = Syscall::invoke(Syscall::create_server_socket, path.data(), 4); rc >= 0) {
        m_fd.emplace(static_cast<uint32>(rc));
    }
    ENSURE(m_fd);
    event_loop.watch(*this, PollEvents::Accept);
    set_on_read_ready([this] {
        uint32 client_fd = 0;
        if (auto rc = Syscall::invoke(Syscall::accept, *m_fd); rc >= 0) {
            client_fd = static_cast<uint32>(rc);
        } else {
            ENSURE_NOT_REACHED();
        }
        auto *client = m_clients.emplace(new Client(client_fd)).obj();
        m_event_loop.watch(*client, PollEvents::Read);
        client->set_on_read_ready([this, client] {
            // NOLINTNEXTLINE
            Array<uint8, 8_KiB> buffer;
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
            MessageDecoder decoder({buffer.data(), bytes_read});
            m_on_read(*client, decoder);
        });
    });
}

Server::~Server() {
    m_event_loop.unwatch(*this);
    for (auto &client : m_clients) {
        m_event_loop.unwatch(*client);
    }
}

} // namespace ipc
