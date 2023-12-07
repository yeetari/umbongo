#include "Device.hh"

#include "Common.hh"
#include "Context.hh"
#include "Descriptor.hh"
#include "Endpoint.hh"
#include "Error.hh"
#include "HostController.hh"
#include "Port.hh"

#include <core/Error.hh>
#include <mmio/Mmio.hh>
#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/Function.hh>
#include <ustd/Optional.hh>
#include <ustd/Result.hh>
#include <ustd/Span.hh>
#include <ustd/Try.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>
#include <ustd/Utility.hh>
#include <ustd/Vector.hh>

Device::Device(HostController &controller, uint8_t slot_id) : m_controller(controller), m_slot_id(slot_id) {}
Device::Device(Device &&other) : m_controller(other.m_controller), m_slot_id(other.m_slot_id) {
    m_context = ustd::exchange(other.m_context, nullptr);
    m_control_endpoint = ustd::exchange(other.m_control_endpoint, nullptr);
    m_endpoints = ustd::move(other.m_endpoints);
    m_configuration = ustd::exchange(other.m_configuration, nullptr);
    m_configuration_length = ustd::exchange(other.m_configuration_length, static_cast<uint16_t>(0));
}

// TODO: Free context and configuration.
Device::~Device() = default;

EndpointContext &Device::endpoint_context(uint32_t endpoint_id) const {
    if (m_controller.context_size() == 32) {
        return static_cast<DeviceContext *>(m_context)->endpoints[endpoint_id];
    }
    return static_cast<LargeDeviceContext *>(m_context)->endpoints[endpoint_id];
}

ustd::Result<void, DeviceError> Device::read_configuration() {
    ControlTransfer transfer{
        .recipient = ControlRecipient::Device,
        .direction = ControlDirection::DeviceToHost,
        .request = ControlRequest::GetDescriptor,
        .value = 1u << 9u, // Config Descriptor
    };
    ConfigDescriptor descriptor{};
    TRY(m_control_endpoint->send_control(transfer, TransferType::ReadData, {&descriptor, sizeof(ConfigDescriptor)}));
    m_configuration = new uint8_t[m_configuration_length = descriptor.total_length];
    TRY(m_control_endpoint->send_control(transfer, TransferType::ReadData, {m_configuration, m_configuration_length}));
    return {};
}

void Device::ring_doorbell(uint32_t endpoint_id) const {
    m_controller.ring_doorbell(m_slot_id, endpoint_id + 1);
}

Endpoint &Device::create_endpoint(uint32_t address) {
    const auto direction = (address & (1u << 7u)) >> 7u;
    const auto id = (address & 0xfu) * 2u + direction - 1u;
    return *m_endpoints.emplace(new Endpoint(*this, id));
}

ustd::Result<void *, ustd::ErrorUnion<DeviceError, HostError, ub_error_t>> Device::setup(const Port &port) {
    switch (m_controller.context_size()) {
    case 32:
        m_context = TRY(mmio::alloc_dma<DeviceContext>());
        static_cast<DeviceContext *>(m_context)->slot.entry_count = 1u;
        static_cast<DeviceContext *>(m_context)->slot.port_id = port.id();
        break;
    case 64:
        m_context = TRY(mmio::alloc_dma<LargeDeviceContext>());
        static_cast<LargeDeviceContext *>(m_context)->slot.entry_count = 1u;
        static_cast<LargeDeviceContext *>(m_context)->slot.port_id = port.id();
        break;
    }

    constexpr ustd::Array<uint16_t, 5> packet_sizes{8, 64, 8, 64, 512};
    if (port.speed() >= packet_sizes.size()) {
        return DeviceError::UnsupportedSpeed;
    }

    auto &control_endpoint = create_endpoint(0x80u);
    if (auto result = control_endpoint.setup(EndpointType::Control, packet_sizes[port.speed()]); result.is_error()) {
        if (auto host_error = result.error().as<HostError>()) {
            return *host_error;
        }
        if (auto sys_error = result.error().as<ub_error_t>()) {
            return *sys_error;
        }
        ENSURE_NOT_REACHED();
    }
    m_control_endpoint = &control_endpoint;
    return m_context;
}

ustd::Result<void, DeviceError> Device::read_descriptor(ustd::Span<uint8_t> descriptor) {
    ControlTransfer transfer{
        .recipient = ControlRecipient::Device,
        .direction = ControlDirection::DeviceToHost,
        .request = ControlRequest::GetDescriptor,
        .value = 1u << 8u,
    };
    TRY(m_control_endpoint->send_control(transfer, TransferType::ReadData, descriptor));
    return {};
}

ustd::Result<void, DeviceError> Device::set_configuration(uint8_t config_value) {
    // TODO: Designated initialiser doesn't work here (compiler bug?)
    ControlTransfer transfer{};
    transfer.recipient = ControlRecipient::Device;
    transfer.direction = ControlDirection::HostToDevice;
    transfer.request = ControlRequest::SetConfiguration;
    transfer.value = config_value;
    TRY(m_control_endpoint->send_control(transfer, TransferType::NoData));
    return {};
}

ustd::Result<void, DeviceError>
Device::walk_configuration(ustd::Function<ustd::Result<void, DeviceError>(void *, DescriptorType)> callback) {
    if (m_configuration == nullptr) {
        TRY(read_configuration());
    }
    for (auto *ptr = m_configuration; ptr < m_configuration + m_configuration_length;) {
        auto *header = reinterpret_cast<DescriptorHeader *>(ptr);
        TRY(callback(header, header->type));
        ptr += header->length;
    }
    return {};
}

TrbRing &Device::endpoint_ring(uint32_t endpoint_id) const {
    for (const auto &endpoint : m_endpoints) {
        if (endpoint->id() == endpoint_id) {
            return endpoint->transfer_ring();
        }
    }
    ENSURE_NOT_REACHED();
}

SlotState Device::slot_state() const {
    if (m_controller.context_size() == 32) {
        return static_cast<DeviceContext *>(m_context)->slot.slot_state;
    }
    return static_cast<LargeDeviceContext *>(m_context)->slot.slot_state;
}
