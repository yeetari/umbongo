#pragma once

#include <kernel/SysResult.hh>
#include <kernel/devices/Device.hh>
#include <ustd/Span.hh> // IWYU pragma: keep
#include <ustd/Types.hh>

namespace pci {

class Function final : public ::Device {
    const uint16 m_segment;
    const uint8 m_bus;
    const uint8 m_device;
    const uint8 m_function;
    uintptr m_address;
    uint16 m_vendor_id;
    uint16 m_device_id;

    template <typename T>
    T read_config(uint32 offset) const;
    uintptr read_bar(uint8 index) const;

public:
    Function(uintptr segment_base, uint16 segment, uint8 bus, uint8 device, uint8 function);

    bool can_read() override { return true; }
    bool can_write() override { return true; }
    SysResult<usize> read(ustd::Span<void> data, usize offset) override;
};

} // namespace pci
