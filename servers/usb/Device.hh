#pragma once

#include "Common.hh"
#include "Error.hh"

#include <core/Error.hh>
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
    const uint8_t m_slot_id;

    void *m_context{nullptr};
    Endpoint *m_control_endpoint{nullptr};
    ustd::Vector<ustd::UniquePtr<Endpoint>> m_endpoints;

    uint8_t *m_configuration{nullptr};
    uint16_t m_configuration_length{0};

    EndpointContext &endpoint_context(uint32_t endpoint_id) const;
    ustd::Result<void, DeviceError> read_configuration();
    void ring_doorbell(uint32_t endpoint_id) const;

public:
    Device(HostController &controller, uint8_t slot_id);
    Device(const Device &) = delete;
    Device(Device &&other);
    virtual ~Device();

    Device &operator=(const Device &) = delete;
    Device &operator=(Device &&) = delete;

    virtual void poll() {}

    Endpoint &create_endpoint(uint32_t address);
    ustd::Result<void *, ustd::ErrorUnion<DeviceError, HostError, core::SysError>> setup(const Port &port);
    ustd::Result<void, DeviceError> read_descriptor(ustd::Span<uint8_t> descriptor);
    ustd::Result<void, DeviceError> set_configuration(uint8_t config_value);
    ustd::Result<void, DeviceError>
    walk_configuration(ustd::Function<ustd::Result<void, DeviceError>(void *, DescriptorType)> callback);

    TrbRing &endpoint_ring(uint32_t endpoint_id) const;
    SlotState slot_state() const;
};
