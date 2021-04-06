#pragma once

#include <kernel/usb/EndpointType.hh>
#include <kernel/usb/TransferType.hh>
#include <ustd/Span.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>

namespace usb {

class Device;
class TrbRing;

struct ControlTransfer;

class Endpoint {
    Device *const m_device;
    const uint8 m_id;
    TrbRing *m_transfer_ring;

public:
    Endpoint(Device *device, uint8 id);
    Endpoint(const Endpoint &) = delete;
    Endpoint(Endpoint &&other) noexcept
        : m_device(other.m_device), m_id(other.m_id), m_transfer_ring(ustd::exchange(other.m_transfer_ring, nullptr)) {}
    ~Endpoint();

    Endpoint &operator=(const Endpoint &) = delete;
    Endpoint &operator=(Endpoint &&) = delete;

    void setup(EndpointType type, uint16 packet_size);
    void send_control(ControlTransfer *transfer, TransferType transfer_type, Span<void> data = {nullptr, 0});
    void setup_interval_input(Span<uint8> buffer, uint8 interval);

    uint8 id() const { return m_id; }
};

} // namespace usb
