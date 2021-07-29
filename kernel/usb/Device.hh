#pragma once

#include <kernel/devices/Device.hh>
#include <kernel/usb/Descriptors.hh>
#include <kernel/usb/Endpoint.hh> // IWYU pragma: keep
#include <kernel/usb/SlotState.hh>
#include <ustd/Assert.hh>
#include <ustd/Span.hh> // IWYU pragma: keep
#include <ustd/Types.hh>
#include <ustd/Utility.hh>
#include <ustd/Vector.hh>

namespace usb {

struct EndpointContext;
struct InputControlContext;
struct SlotContext;
class HostController;
class Port;

class Device : public ::Device {
    friend Endpoint;

private:
    HostController *const m_controller;
    const uint8 m_slot;
    void *m_context;

    uint8 *m_configuration{nullptr};
    uint16 m_configuration_length{0};

    Endpoint *m_control_endpoint{nullptr};
    Vector<Endpoint> m_endpoints;

    EndpointContext &endpoint_context(uint8 endpoint);
    InputControlContext &input_context();
    void ring_doorbell(uint8 endpoint);

public:
    Device(HostController *controller, uint8 slot);
    Device(const Device &) = delete;
    Device(Device &&other) noexcept
        : m_controller(other.m_controller), m_slot(other.m_slot), m_context(ustd::exchange(other.m_context, nullptr)),
          m_configuration(ustd::exchange(other.m_configuration, nullptr)),
          m_configuration_length(other.m_configuration_length),
          m_control_endpoint(ustd::exchange(other.m_control_endpoint, nullptr)),
          m_endpoints(ustd::move(other.m_endpoints)) {}
    ~Device() override;

    Device &operator=(const Device &) = delete;
    Device &operator=(Device &&) = delete;

    virtual void poll() {}

    Endpoint *create_endpoint(uint8 address);
    void setup(const Port &port);
    void read_descriptor(Span<uint8> data);
    void read_configuration();
    void set_configuration(uint8 config_value);
    void configure_endpoint(Endpoint *endpoint);

    template <typename F>
    void walk_configuration(F callback) {
        ASSERT(m_configuration != nullptr && m_configuration_length != 0);
        auto *ptr = m_configuration;
        auto *end = ptr + m_configuration_length;
        while (ptr < end) {
            auto *header = reinterpret_cast<DescriptorHeader *>(ptr);
            callback(static_cast<void *>(header), header->type);
            ptr += header->length;
        }
    }

    SlotContext &slot_context();

    uint8 slot() const { return m_slot; }
    void *context() const { return m_context; }
    SlotState slot_state() const;
};

} // namespace usb
