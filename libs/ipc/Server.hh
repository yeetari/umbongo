#pragma once

#include <core/Watchable.hh>
#include <ipc/Client.hh> // IWYU pragma: keep
#include <ustd/Function.hh>
#include <ustd/Optional.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh> // IWYU pragma: keep
#include <ustd/Utility.hh>
#include <ustd/Vector.hh>

namespace core {

class EventLoop;

} // namespace core

namespace ipc {

class MessageDecoder;

class Server : public core::Watchable {
    core::EventLoop &m_event_loop;
    ustd::Optional<uint32> m_fd;
    ustd::Vector<ustd::UniquePtr<Client>> m_clients;
    ustd::Function<void(Client &, MessageDecoder &)> m_on_read;

public:
    Server(core::EventLoop &event_loop, ustd::StringView path);
    Server(const Server &) = delete;
    Server(Server &&) = delete;
    ~Server() override;

    Server &operator=(const Server &) = delete;
    Server &operator=(Server &&) = delete;

    void set_on_read(ustd::Function<void(Client &, MessageDecoder &)> on_read) { m_on_read = ustd::move(on_read); }

    uint32 fd() const override { return *m_fd; }
};

} // namespace ipc
