#pragma once

#include "Common.hh"
#include "Error.hh"

#include <kernel/SysError.hh>
#include <ustd/Function.hh> // IWYU pragma: keep
#include <ustd/Result.hh>
#include <ustd/Span.hh> // IWYU pragma: keep
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh> // IWYU pragma: keep
#include <ustd/Vector.hh>

class Endpoint;
struct EndpointContext;
class HostController;
class Port;
class TrbRing;

class Device {
    friend Endpoint;

private:
    HostController &m_controller;
    const uint8 m_slot_id;

    void *m_context{nullptr};
    Endpoint *m_control_endpoint{nullptr};
    ustd::Vector<ustd::UniquePtr<Endpoint>> m_endpoints;

    uint8 *m_configuration{nullptr};
    uint16 m_configuration_length{0};

    EndpointContext &endpoint_context(uint32 endpoint_id) const;
    ustd::Result<void, DeviceError> read_configuration();
    void ring_doorbell(uint32 endpoint_id) const;

public:
    Device(HostController &controller, uint8 slot_id);
    Device(const Device &) = delete;
    Device(Device &&other) noexcept;
    virtual ~Device();

    Device &operator=(const Device &) = delete;
    Device &operator=(Device &&) = delete;

    virtual void poll() {}

    Endpoint &create_endpoint(uint32 address);
    ustd::Result<void *, ustd::ErrorUnion<DeviceError, HostError, SysError>> setup(const Port &port);
    ustd::Result<void, DeviceError> read_descriptor(ustd::Span<uint8> descriptor);
    ustd::Result<void, DeviceError> set_configuration(uint8 config_value);
    ustd::Result<void, DeviceError>
    walk_configuration(ustd::Function<ustd::Result<void, DeviceError>(void *, DescriptorType)> callback);

    TrbRing &endpoint_ring(uint32 endpoint_id) const;
    SlotState slot_state() const;
};
