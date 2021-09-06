#include <ipc/Client.hh>

#include <core/Error.hh>
#include <ipc/Message.hh>
#include <ipc/MessageEncoder.hh>
#include <kernel/Syscall.hh>
#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/Log.hh>
#include <ustd/Optional.hh>
#include <ustd/Span.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>

namespace ipc {

Client::~Client() {
    if (m_fd) {
        Syscall::invoke(Syscall::close, *m_fd);
    }
}

bool Client::connect(StringView path) {
    if (auto rc = Syscall::invoke(Syscall::connect, path.data()); rc >= 0) {
        m_fd.emplace(static_cast<uint32>(rc));
    } else {
        dbgln("Could not connect to {}: {}", path, core::error_string(rc));
        return false;
    }
    return true;
}

void Client::send_message(const Message &message) {
    // NOLINTNEXTLINE
    Array<uint8, 8_KiB> buffer;
    MessageEncoder encoder(buffer.span());
    message.encode(encoder);
    [[maybe_unused]] auto bytes_written = Syscall::invoke(Syscall::write, *m_fd, buffer.data(), encoder.size());
    ASSERT(bytes_written >= 0);
    ASSERT(static_cast<usize>(bytes_written) == encoder.size());
}

usize Client::wait_message(Span<uint8> buffer) {
    auto bytes_read = Syscall::invoke(Syscall::read, *m_fd, buffer.data(), buffer.size());
    ASSERT(bytes_read >= 0);
    return static_cast<usize>(bytes_read);
}

} // namespace ipc
