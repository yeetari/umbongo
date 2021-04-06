#include <kernel/usb/Device.hh>

#include <kernel/usb/Contexts.hh>
#include <kernel/usb/Descriptors.hh>
#include <kernel/usb/Endpoint.hh>
#include <kernel/usb/HostController.hh>
#include <kernel/usb/Transfers.hh>

namespace usb {

Device::Device(HostController *controller, uint8 slot)
    : m_controller(controller), m_slot(slot), m_context(new DeviceContext{}) {}

Device::~Device() {
    delete m_configuration;
    delete m_context;
}

void Device::ring_doorbell(uint8 endpoint) {
    m_controller->ring_doorbell(m_slot, endpoint + 1);
}

Endpoint *Device::create_endpoint(uint8 address) {
    const bool output = (address & (1u << 7u)) == 0;
    uint8 index = address * 2 - static_cast<uint8>(output);
    return &m_endpoints.emplace(this, static_cast<uint8>(index & 0xfu));
}

void Device::setup(const Port &port) {
    // We're enabling the slot context and the default control endpoint.
    m_context->input.enable = 0b11u;

    // Setup slot context.
    auto &slot_context = m_context->slot;
    slot_context.entry_count = 1;
    slot_context.speed = port.speed();
    slot_context.port_num = port.socket() + 1;

    // Create control endpoint.
    constexpr Array<uint16, 5> packet_sizes{8, 8, 8, 64, 512};
    const uint16 packet_size = packet_sizes[port.speed()];
    m_control_endpoint = &m_endpoints.emplace(this, static_cast<uint8>(0));
    m_control_endpoint->setup(EndpointType::Control, packet_size);
}

void Device::read_descriptor(Span<uint8> data) {
    ControlTransfer transfer{
        .recipient = ControlRecipient::Device,
        .type = ControlType::Standard,
        .direction = ControlDirection::DeviceToHost,
        .request = ControlRequest::GetDescriptor,
        .value = 1u << 8u, // Device Descriptor
    };
    m_control_endpoint->send_control(&transfer, TransferType::ReadData, data);
}

void Device::read_configuration() {
    ConfigDescriptor config_descriptor{};
    ControlTransfer transfer{
        .recipient = ControlRecipient::Device,
        .type = ControlType::Standard,
        .direction = ControlDirection::DeviceToHost,
        .request = ControlRequest::GetDescriptor,
        .value = 1u << 9u, // Config Descriptor
    };
    m_control_endpoint->send_control(&transfer, TransferType::ReadData, {&config_descriptor, sizeof(ConfigDescriptor)});
    m_configuration_length = config_descriptor.total_length;
    m_configuration = new uint8[m_configuration_length];
    m_control_endpoint->send_control(&transfer, TransferType::ReadData, {m_configuration, m_configuration_length});
}

void Device::set_configuration(uint8 config_value) {
    ControlTransfer transfer{
        .recipient = ControlRecipient::Device,
        .type = ControlType::Standard,
        .direction = ControlDirection::HostToDevice,
        .request = ControlRequest::SetConfiguration,
        .value = config_value,
    };
    m_control_endpoint->send_control(&transfer, TransferType::NoData);
}

void Device::configure_endpoint(Endpoint *endpoint) {
    m_context->input.disable = 0;
    m_context->input.enable = 1u << (endpoint->id() + 1u) | 1u;
    m_context->slot.entry_count++;
    TransferRequestBlock configure_endpoint_cmd{
        .data_ptr = m_context,
        .type = TrbType::ConfigureEndpoint,
        .slot = m_slot,
    };
    m_controller->send_command(&configure_endpoint_cmd);
    ring_doorbell(endpoint->id());
}

SlotState Device::slot_state() const {
    return m_context->slot.slot_slate;
}

} // namespace usb
