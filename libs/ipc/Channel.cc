#include <ipc/Channel.hh>

#include <system/Syscall.hh>
#include <ustd/Try.hh>

namespace ipc {

ustd::Result<Channel, ub_error_t> create_channel(const core::Handle &endpoint_handle, size_t buffer_size) {
    auto vmo_handle = core::wrap_handle(TRY(system::syscall<ub_handle_t>(SYS_vmo_create, buffer_size)));
    auto chnl_handle =
        core::wrap_handle(TRY(system::syscall<ub_handle_t>(SYS_chnl_create, *endpoint_handle, *vmo_handle)));
    return Channel(ustd::move(chnl_handle));
}

ustd::Result<void, ub_error_t> Channel::send(ustd::Span<const uint8_t> message, ustd::Span<ub_handle_t> handles) {
    TRY(system::syscall(SYS_chnl_send, *m_handle, message.data(), message.size(), handles.data(), handles.size()));
    return {};
}

} // namespace ipc
