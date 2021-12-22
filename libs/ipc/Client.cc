#include <ipc/Client.hh>

#include <core/Error.hh>
#include <ipc/Message.hh>
#include <ipc/MessageEncoder.hh>
#include <kernel/Syscall.hh>
#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/Log.hh>
#include <ustd/Optional.hh>
#include <ustd/Result.hh>
#include <ustd/Span.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>

namespace ipc {

// TODO: Better error propagation here.

Client::~Client() {
    if (m_fd) {
        EXPECT(Syscall::invoke(Syscall::close, *m_fd));
    }
}

bool Client::connect(ustd::StringView path) {
    if (auto result = Syscall::invoke(Syscall::connect, path.data()); !result.is_error()) {
        m_fd.emplace(static_cast<uint32>(result.value()));
    } else {
        dbgln("Could not connect to {}: {}", path, core::error_string(result.error()));
        return false;
    }
    return true;
}

void Client::send_message(const Message &message) {
    // NOLINTNEXTLINE
    ustd::Array<uint8, 8_KiB> buffer;
    MessageEncoder encoder(buffer.span());
    message.encode(encoder);
    [[maybe_unused]] auto bytes_written = Syscall::invoke(Syscall::write, *m_fd, buffer.data(), encoder.size());
    ASSERT(!bytes_written.is_error());
    ASSERT(bytes_written.value() == encoder.size());
}

usize Client::wait_message(ustd::Span<uint8> buffer) {
    auto bytes_read = Syscall::invoke(Syscall::read, *m_fd, buffer.data(), buffer.size());
    ASSERT(!bytes_read.is_error());
    return bytes_read.value();
}

} // namespace ipc
