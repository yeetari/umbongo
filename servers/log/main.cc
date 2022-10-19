#include <core/Debug.hh>
#include <core/Time.hh>
#include <ipc/Channel.hh>
#include <ipc/Decoder.hh>
#include <ipc/Encoder.hh>
#include <ipc/Endpoint.hh>
#include <ustd/Array.hh>
#include <ustd/Format.hh>
#include <ustd/Optional.hh>
#include <ustd/String.hh>
#include <ustd/StringView.hh>
#include <ustd/Try.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>
#include <ustd/Vector.hh>

size_t main() {
    auto endpoint = EXPECT(ipc::create_endpoint());

    ipc::Encoder encoder;
    encoder.encode(uint32_t(1));
    encoder.encode("log-service"sv);

    auto root_server_channel = EXPECT(ipc::create_channel(core::wrap_handle(0)));
    ub_handle_t handle_to_send = *endpoint.handle();
    EXPECT(root_server_channel.send(encoder.span(), {&handle_to_send, 1}));

    while (true) {
        auto [decoder, handles] = EXPECT(endpoint.recv());
        core::debug_line(EXPECT(decoder.decode<ustd::StringView>()));
    }
}
