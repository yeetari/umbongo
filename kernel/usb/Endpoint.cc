#include <kernel/usb/Endpoint.hh>

#include <kernel/proc/Scheduler.hh>
#include <kernel/usb/Contexts.hh>
#include <kernel/usb/Device.hh>
#include <kernel/usb/EndpointType.hh>
#include <kernel/usb/TransferType.hh>
#include <kernel/usb/Transfers.hh>
#include <kernel/usb/TrbRing.hh>
#include <ustd/Assert.hh>
#include <ustd/Span.hh>
#include <ustd/Types.hh>

namespace usb {
namespace {

uint8 calculate_interval(uint8 interval) {
    for (uint8 i = 0; i < 8; i++) {
        if ((interval & (0x80u >> i)) != 0) {
            return 7 - i;
        }
    }
    return 7;
}

} // namespace

Endpoint::Endpoint(Device *device, uint8 id) : m_device(device), m_id(id), m_transfer_ring(new TrbRing(true)) {}

Endpoint::~Endpoint() {
    delete m_transfer_ring;
}

void Endpoint::setup(EndpointType type, uint16 packet_size) {
    auto &context = m_device->endpoint_context(m_id);
    context.ep_type = type;
    context.max_packet_size = packet_size;
    context.error_count = 3;
    context.average_trb_length = type == EndpointType::Control ? 8 : packet_size * 2;
    context.tr_dequeue_ptr = reinterpret_cast<uint64>(m_transfer_ring) | 1u;
}

void Endpoint::send_control(ControlTransfer *transfer, TransferType transfer_type, Span<void> data) {
    ASSERT(data.size() < 65535);
    transfer->length = static_cast<uint16>(data.size());

    // Control transfer is 8 bytes and therefore can fit directly into the data field of the TRB. We also need to set
    // the immediate data (IDT) flag.
    TransferRequestBlock setup_trb{
        .data = *reinterpret_cast<uint64 *>(transfer),
        .status = 8,
        .idt = true,
        .type = TrbType::SetupStage,
        .transfer_type = transfer_type,
    };
    m_transfer_ring->enqueue(&setup_trb);

    TransferRequestBlock data_trb{
        .data_ptr = data.data(),
        .status = transfer->length,
        .type = TrbType::DataStage,
    };
    if (transfer_type != TransferType::NoData) {
        ASSERT(data.data() != nullptr && data.size() != 0);
        data_trb.transfer_direction =
            transfer_type == TransferType::ReadData ? TransferDirection::Read : TransferDirection::Write;
        m_transfer_ring->enqueue(&data_trb);
    }

    TransferRequestBlock status_trb{
        .chain = true,
        .type = TrbType::StatusStage,
    };
    m_transfer_ring->enqueue(&status_trb);

    TransferRequestBlock event_data_trb{
        .data_ptr = &status_trb,
        .ioc = true,
        .type = TrbType::EventData,
    };
    m_transfer_ring->enqueue(&event_data_trb);

    m_device->ring_doorbell(m_id);
    while ((status_trb.status & (1u << 31u)) == 0) {
        Scheduler::wait(1);
    }
}

void Endpoint::setup_interval_input(Span<uint8> buffer, uint8 interval) {
    m_device->endpoint_context(m_id).interval = calculate_interval(interval);
    TransferRequestBlock in_trb{
        .data_ptr = buffer.data(),
        .status = static_cast<uint16>(buffer.size()),
        .ioc = true,
        .type = TrbType::Normal,
    };
    for (usize i = 0; i < 255; i++) {
        m_transfer_ring->enqueue(&in_trb);
    }
}

} // namespace usb
