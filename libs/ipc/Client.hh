#pragma once

#include <core/Watchable.hh>
#include <ustd/Array.hh>
#include <ustd/Optional.hh>
#include <ustd/Span.hh> // IWYU pragma: keep
#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>

namespace ipc {

class Message;

class Client : public core::Watchable {
    ustd::Optional<uint32> m_fd;

public:
    explicit Client(ustd::Optional<uint32> fd = {}) : m_fd(ustd::move(fd)) {}
    Client(const Client &) = delete;
    Client(Client &&other) noexcept : m_fd(ustd::move(other.m_fd)) {}
    ~Client() override;

    Client &operator=(const Client &) = delete;
    Client &operator=(Client &&) = delete;

    bool connect(ustd::StringView path);
    void send_message(const Message &message);
    usize wait_message(ustd::Span<uint8> buffer);

    template <typename T, typename... Args>
    void send_message(Args &&...args);
    template <typename T>
    T wait_message();

    bool connected() const { return m_fd.has_value(); }
    uint32 fd() const override { return *m_fd; }
};

template <typename T, typename... Args>
void Client::send_message(Args &&...args) {
    send_message(T(ustd::forward<Args>(args)...));
}

template <typename T>
T Client::wait_message() {
    // NOLINTNEXTLINE
    ustd::Array<uint8, 8_KiB> buffer;
    usize bytes_read = wait_message(buffer.span());
    return T::decode({buffer.data(), bytes_read});
}

} // namespace ipc
