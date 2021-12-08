#include <kernel/pci/Function.hh>

#include <kernel/SysResult.hh>
#include <kernel/SyscallTypes.hh>
#include <kernel/devices/Device.hh>
#include <ustd/Format.hh>
#include <ustd/Memory.hh>
#include <ustd/Numeric.hh>
#include <ustd/Span.hh>
#include <ustd/Types.hh>

namespace pci {

Function::Function(uintptr segment_base, uint16 segment, uint8 bus, uint8 device, uint8 function)
    : Device(ustd::format("pci/{:x4}:{:x2}:{:x2}.{}", segment, bus, device, function)), m_segment(segment), m_bus(bus),
      m_device(device), m_function(function) {
    m_address = segment_base;
    m_address += static_cast<uintptr>(bus) << 20ul;
    m_address += static_cast<uintptr>(device) << 15ul;
    m_address += static_cast<uintptr>(function) << 12ul;
    m_vendor_id = read_config<uint16>(0);
    m_device_id = read_config<uint16>(2);
}

template <typename T>
T Function::read_config(uint32 offset) const {
    return *reinterpret_cast<volatile T *>(m_address + offset);
}

uintptr Function::read_bar(uint8 index) const {
    auto bar = static_cast<uintptr>(read_config<uint32>(0x10 + index * sizeof(uint32)));
    if (((bar >> 1u) & 0b11u) == 2) {
        // BAR is a 64-bit address.
        auto bar_ext = read_config<uint32>(0x10 + (index + 1) * sizeof(uint32));
        bar |= static_cast<uintptr>(bar_ext) << 32u;
    }
    return bar & ~0xful;
}

SysResult<usize> Function::read(ustd::Span<void> data, usize offset) {
    if (offset != 0) {
        return 0u;
    }
    PciDeviceInfo info{
        .bars{
            read_bar(0),
            read_bar(1),
            read_bar(2),
            read_bar(3),
            read_bar(4),
            read_bar(5),
        },
        .vendor_id = m_vendor_id,
        .device_id = m_device_id,
    };
    const auto size = ustd::min(data.size(), sizeof(PciDeviceInfo));
    memcpy(data.data(), &info, size);
    return size;
}

} // namespace pci
