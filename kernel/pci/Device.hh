#pragma once

#include <ustd/Types.hh>

namespace pci {

class Bus;

class Device {
    const Bus *const m_bus;
    const uint8 m_device;
    const uint8 m_function;

    uintptr address_for(uint32 offset) const;

public:
    Device(const Bus *bus, uint8 device, uint8 function) : m_bus(bus), m_device(device), m_function(function) {}
    Device(const Device &) = delete;
    Device(Device &&) noexcept = default;
    virtual ~Device() = default;

    Device &operator=(const Device &) = delete;
    Device &operator=(Device &&) = delete;

    template <typename T>
    T read_config(uint32 offset) const;

    template <typename T>
    void write_config(uint32 offset, T value);

    void configure();
    uintptr read_bar(uint8 index) const;

    template <typename F>
    void walk_capabilities(F callback) const;

    void enable_msix(uint8 vector);
    bool msix_supported() const;

    uint16 vendor_id() const;
    uint16 device_id() const;
    uint32 class_info() const;
};

extern template uint8 Device::read_config(uint32) const;
extern template uint16 Device::read_config(uint32) const;
extern template uint32 Device::read_config(uint32) const;

extern template void Device::write_config(uint32, uint8);
extern template void Device::write_config(uint32, uint16);
extern template void Device::write_config(uint32, uint32);

} // namespace pci
