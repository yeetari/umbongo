#include "endpoint.hh"

#include "common.hh"
#include "context.hh"
#include "device.hh"
#include "error.hh"
#include "host_controller.hh"
#include "trb_ring.hh"

#include <core/error.hh>
#include <core/time.hh>
#include <mmio/mmio.hh>
#include <ustd/result.hh>
#include <ustd/span.hh>
#include <ustd/try.hh>
#include <ustd/types.hh>
#include <ustd/unique_ptr.hh>

Endpoint::Endpoint(const Device &device, uint32_t id) : m_device(device), m_id(id) {}
Endpoint::~Endpoint() = default;

ustd::Result<void, ustd::ErrorUnion<HostError, ub_error_t>> Endpoint::setup(EndpointType type, uint16_t packet_size) {
    auto &context = m_device.endpoint_context(m_id);
    m_transfer_ring = TRY(TrbRing::create(true));
    context.endpoint_type = type;
    context.max_packet_size = packet_size;
    context.tr_dequeue_pointer = EXPECT(m_transfer_ring->physical_base()) | 1u;
    context.error_count = 3u;
    if (m_id != 0u) {
        // TODO: Free input_context.
        void *input_context_ptr = nullptr;
        switch (m_device.m_controller.context_size()) {
        case 32: {
            static_cast<DeviceContext *>(m_device.m_context)->slot.entry_count++;
            auto *input_context = TRY(mmio::alloc_dma<InputContext>());
            input_context->control.add = (1u << (m_id + 1u)) | 1u;
            __builtin_memcpy(&input_context->device, m_device.m_context, sizeof(DeviceContext));
            input_context_ptr = input_context;
            break;
        }
        case 64: {
            static_cast<LargeDeviceContext *>(m_device.m_context)->slot.entry_count++;
            auto *input_context = TRY(mmio::alloc_dma<LargeInputContext>());
            input_context->control.add = (1u << (m_id + 1u)) | 1u;
            __builtin_memcpy(&input_context->device, m_device.m_context, sizeof(LargeDeviceContext));
            input_context_ptr = input_context;
        }
        }
        TRY(m_device.m_controller.send_command({
            .data = EXPECT(mmio::virt_to_phys(input_context_ptr)),
            .type = TrbType::ConfigureEndpointCmd,
            .slot_id = m_device.m_slot_id,
        }));
    }
    return {};
}

ustd::Result<void, DeviceError> Endpoint::send_control(ControlTransfer transfer, TransferType transfer_type,
                                                       ustd::Span<void> data) {
    transfer.length = static_cast<uint16_t>(data.size());

    // Control transfer is 8 bytes and can therefore fit directly into the data field of the setup TRB via the immediate
    // data flag.
    m_transfer_ring->enqueue({
        .data = __builtin_bit_cast(uint64_t, transfer),
        .status = sizeof(uint64_t),
        .immediate_data = true,
        .type = TrbType::SetupStage,
        .transfer_type = transfer_type,
    });

    if (transfer_type != TransferType::NoData) {
        m_transfer_ring->enqueue({
            .data = EXPECT(mmio::virt_to_phys(data.data())),
            .status = transfer.length,
            .type = TrbType::DataStage,
            .read = transfer_type == TransferType::ReadData,
        });
    }

    auto &status_trb = m_transfer_ring->enqueue({
        .chain = true,
        .type = TrbType::StatusStage,
    });
    m_transfer_ring->enqueue({
        .data = EXPECT(mmio::virt_to_phys(&status_trb)),
        .event_on_completion = true,
        .type = TrbType::EventData,
    });
    m_device.ring_doorbell(m_id);
    for (size_t timeout = 100; (mmio::read(status_trb.status) & (1u << 31u)) != (1u << 31u); timeout--) {
        if (timeout == 0) {
            return DeviceError::TransferTimedOut;
        }
        // TODO: Better solution for this.
        m_device.m_controller.handle_interrupt();
        core::sleep(1000000ul);
    }
    return {};
}

void Endpoint::set_interval(uint8_t interval) {
    auto &context = m_device.endpoint_context(m_id);
    context.interval = 7u;
    for (uint8_t i = 0; i < 8; i++) {
        if ((interval & (0x80u >> i)) != 0u) {
            context.interval = 7u - i;
            break;
        }
    }
}
