#include <ipc/Client.hh>

#include <core/Error.hh>
#include <core/Syscall.hh>
#include <ipc/Message.hh>
#include <ipc/MessageDecoder.hh>
#include <ipc/MessageEncoder.hh>
#include <log/Log.hh>
#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/Function.hh>
#include <ustd/Optional.hh>
#include <ustd/Result.hh>
#include <ustd/Span.hh>
#include <ustd/StringView.hh>
#include <ustd/Try.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>

namespace ipc {

// TODO: Better error propagation here.

Client::Client(ustd::Optional<uint32> fd) : m_fd(ustd::move(fd)) {
    m_on_disconnect = [this] {
        log::warn("Lost connection to server!");
        if (m_fd) {
            EXPECT(core::syscall(Syscall::close, *m_fd));
        }
    };
    set_on_read_ready([this] {
        // NOLINTNEXTLINE
        ustd::Array<uint8, 8_KiB> buffer;
        usize bytes_read = wait_message(buffer.span());
        if (bytes_read == 0) {
            m_on_disconnect();
            return;
        }
        for (usize position = 0u; position < bytes_read;) {
            MessageDecoder decoder({buffer.data() + position, bytes_read});
            if (!m_on_message(decoder)) {
                m_on_disconnect();
                return;
            }
            if (decoder.bytes_decoded() == 0u) {
                return;
            }
            position += decoder.bytes_decoded();
        }
    });
}

Client::~Client() {
    if (m_fd) {
        EXPECT(core::syscall(Syscall::close, *m_fd));
    }
}

bool Client::connect(ustd::StringView path) {
    if (auto result = core::syscall(Syscall::connect, path.data()); !result.is_error()) {
        m_fd.emplace(static_cast<uint32>(result.value()));
    } else {
        log::error("Could not connect to {}: {}", path, core::error_string(result.error()));
        return false;
    }
    return true;
}

void Client::send_message(const Message &message) {
    // NOLINTNEXTLINE
    ustd::Array<uint8, 8_KiB> buffer;
    MessageEncoder encoder(buffer.span());
    message.encode(encoder);
    [[maybe_unused]] auto bytes_written = core::syscall(Syscall::write, *m_fd, buffer.data(), encoder.size());
    ASSERT(!bytes_written.is_error());
    ASSERT(bytes_written.value() == encoder.size());
}

usize Client::wait_message(ustd::Span<uint8> buffer) {
    auto bytes_read = core::syscall(Syscall::read, *m_fd, buffer.data(), buffer.size());
    ASSERT(!bytes_read.is_error());
    return bytes_read.value();
}

} // namespace ipc
