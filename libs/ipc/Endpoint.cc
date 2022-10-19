#include <ipc/Endpoint.hh>

#include <system/Syscall.hh>
#include <ustd/Try.hh>

namespace ipc {

ustd::Result<Endpoint, ub_error_t> create_endpoint(size_t buffer_size) {
    uint8_t *buffer = nullptr;
    auto vmr_handle = core::wrap_handle(TRY(system::syscall<ub_handle_t>(SYS_vmr_allocate, buffer_size, &buffer)));
    auto endp_handle = core::wrap_handle(TRY(system::syscall<ub_handle_t>(SYS_endp_create, *vmr_handle)));
    return Endpoint(ustd::move(endp_handle), ustd::move(vmr_handle), buffer, buffer_size);
}

ustd::Result<ustd::Tuple<Decoder, ustd::Vector<core::Handle>>, ub_error_t> Endpoint::recv() const {
    ustd::Array<ub_handle_t, UB_MAX_HANDLE_COUNT> handles{};
    const auto handle_count = TRY(system::syscall<uint32_t>(SYS_endp_recv, *m_handle, handles.data()));

    // TODO: ustd::transform function.
    ustd::Vector<core::Handle> wrapped_handles;
    for (uint32_t i = 0; i < handle_count; i++) {
        wrapped_handles.push(core::wrap_handle(handles[i]));
    }

    Decoder decoder({m_buffer, m_buffer_size});
    return ustd::make_tuple(decoder, ustd::move(wrapped_handles));
}

} // namespace ipc
