#include <ipc/Client.hh>

#include <core/Error.hh>
#include <core/Time.hh>
#include <ipc/Message.hh>
#include <ipc/MessageDecoder.hh>
#include <ipc/MessageEncoder.hh>
#include <log/Log.hh>
#include <system/Syscall.hh>
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

Client::Client(ustd::Optional<uint32_t> fd) : m_fd(ustd::move(fd)) {
    m_on_disconnect = [this] {
        log::warn("Lost connection to server!");
        if (m_fd) {
            EXPECT(system::syscall(UB_SYS_close, *m_fd));
        }
    };
    set_on_read_ready([this] {
        // NOLINTNEXTLINE
        ustd::Array<uint8_t, 8_KiB> buffer;
        size_t bytes_read = wait_message(buffer.span());
        if (bytes_read == 0) {
            m_on_disconnect();
            return;
        }
        for (size_t position = 0u; position < bytes_read;) {
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
        EXPECT(system::syscall(UB_SYS_close, *m_fd));
    }
}

bool Client::connect(ustd::StringView path) {
    // TODO: Retries are only a temporary fix. system-server should have proper dependencies/socket takeover.
    for (size_t tries = 100; tries != 0; tries--) {
        auto result = system::syscall(UB_SYS_connect, path.data());
        if (result.is_error() && result.error() == UB_ERROR_NON_EXISTENT) {
            core::sleep(2000000ul);
            continue;
        }
        if (!result.is_error()) {
            m_fd.emplace(static_cast<uint32_t>(result.value()));
            return true;
        }
        log::error("Could not connect to {}: {}", path, core::error_string(result.error()));
        return false;
    }
    log::error("Could not connect to {}: timed out", path);
    return false;
}

void Client::send_message(const Message &message) {
    // NOLINTNEXTLINE
    ustd::Array<uint8_t, 8_KiB> buffer;
    MessageEncoder encoder(buffer.span());
    message.encode(encoder);
    [[maybe_unused]] auto bytes_written = system::syscall(UB_SYS_write, *m_fd, buffer.data(), encoder.size());
    ASSERT(!bytes_written.is_error());
    ASSERT(bytes_written.value() == encoder.size());
}

size_t Client::wait_message(ustd::Span<uint8_t> buffer) {
    auto bytes_read = system::syscall(UB_SYS_read, *m_fd, buffer.data(), buffer.size());
    ASSERT(!bytes_read.is_error());
    return bytes_read.value();
}

} // namespace ipc
