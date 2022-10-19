#pragma once

#include <core/Handle.hh>
#include <ipc/Decoder.hh>
#include <system/Types.h>
#include <ustd/Result.hh>
#include <ustd/Span.hh>
#include <ustd/Tuple.hh>
#include <ustd/Vector.hh>

namespace ipc {

class Endpoint {
    core::Handle m_handle;
    core::Handle m_vmr_handle;
    uint8_t *const m_buffer;
    const size_t m_buffer_size;

public:
    Endpoint(core::Handle &&handle, core::Handle &&vmr_handle, uint8_t *buffer, size_t buffer_size)
        : m_handle(ustd::move(handle)), m_vmr_handle(ustd::move(vmr_handle)), m_buffer(buffer),
          m_buffer_size(buffer_size) {}

    ustd::Result<ustd::Tuple<Decoder, ustd::Vector<core::Handle>>, ub_error_t> recv() const;
    const core::Handle &handle() const { return m_handle; }
    void *buffer() const { return m_buffer; }
};

ustd::Result<Endpoint, ub_error_t> create_endpoint(size_t buffer_size = 4_KiB);

} // namespace ipc
