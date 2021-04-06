#pragma once

#include <ustd/Types.hh>

namespace pci {

class Bus;

class Device {
    const Bus *const m_bus;
    const uint8 m_device;
    const uint8 m_function;

    uintptr address_for(uint16 offset) const;

public:
    Device(const Bus *bus, uint8 device, uint8 function) : m_bus(bus), m_device(device), m_function(function) {}
    Device(const Device &) = delete;
    Device(Device &&) noexcept = default;
    virtual ~Device() = default;

    Device &operator=(const Device &) = delete;
    Device &operator=(Device &&) = delete;

    template <typename T>
    T read_config(uint16 offset) const;

    template <typename T>
    void write_config(uint16 offset, T value);

    virtual void enable();
    uintptr read_bar(uint8 index) const;

    uint16 vendor_id() const;
    uint16 device_id() const;
    uint32 class_info() const;
};

} // namespace pci
