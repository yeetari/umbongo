#pragma once

#include <core/Handle.hh>
#include <system/Types.h>
#include <ustd/Result.hh>
#include <ustd/Span.hh>

namespace ipc {

class Channel {
    core::Handle m_handle;

public:
    explicit Channel(core::Handle &&handle) : m_handle(ustd::move(handle)) {}

    ustd::Result<void, ub_error_t> send(ustd::Span<const uint8_t> message, ustd::Span<ub_handle_t> handles);
};

ustd::Result<Channel, ub_error_t> create_channel(const core::Handle &endpoint_handle, size_t buffer_size = 4_KiB);

} // namespace ipc
