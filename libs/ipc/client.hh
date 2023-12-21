#pragma once

#include <core/watchable.hh>
#include <ustd/array.hh>
#include <ustd/function.hh>
#include <ustd/optional.hh>
#include <ustd/span.hh> // IWYU pragma: keep
#include <ustd/string_view.hh>
#include <ustd/types.hh>
#include <ustd/utility.hh>

namespace ipc {

class Message;
class MessageDecoder;

class Client : public core::Watchable {
    ustd::Optional<uint32_t> m_fd;
    ustd::Function<void()> m_on_disconnect{};
    ustd::Function<bool(MessageDecoder &)> m_on_message{};

public:
    explicit Client(ustd::Optional<uint32_t> fd = {});
    Client(const Client &) = delete;
    Client(Client &&other)
        : m_fd(ustd::move(other.m_fd)), m_on_disconnect(ustd::move(other.m_on_disconnect)),
          m_on_message(ustd::move(other.m_on_message)) {}
    ~Client() override;

    Client &operator=(const Client &) = delete;
    Client &operator=(Client &&) = delete;

    bool connect(ustd::StringView path);
    void send_message(const Message &message);
    size_t wait_message(ustd::Span<uint8_t> buffer);

    template <typename T, typename... Args>
    void send_message(Args &&...args);
    template <typename T>
    T wait_message();

    void set_on_disconnect(ustd::Function<void()> on_disconnect) { m_on_disconnect = ustd::move(on_disconnect); }
    void set_on_message(ustd::Function<bool(MessageDecoder &)> on_message) { m_on_message = ustd::move(on_message); }

    bool connected() const { return m_fd.has_value(); }
    uint32_t fd() const override { return *m_fd; }
};

template <typename T, typename... Args>
void Client::send_message(Args &&...args) {
    send_message(T(ustd::forward<Args>(args)...));
}

template <typename T>
T Client::wait_message() {
    // NOLINTNEXTLINE
    ustd::Array<uint8_t, 8_KiB> buffer;
    size_t bytes_read = wait_message(buffer.span());
    return T::decode({buffer.data(), bytes_read});
}

} // namespace ipc
