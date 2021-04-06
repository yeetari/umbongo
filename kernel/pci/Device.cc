#include <kernel/pci/Device.hh>

#include <kernel/pci/Bus.hh>
#include <ustd/Assert.hh>
#include <ustd/Types.hh>

namespace pci {

uintptr Device::address_for(uint16 offset) const {
    uintptr address = m_bus->segment_base();
    address += static_cast<uintptr>(m_bus->num()) << 20ul;
    address |= static_cast<uintptr>(m_device) << 15ul;
    address |= static_cast<uintptr>(m_function) << 12ul;
    address |= static_cast<uintptr>(offset);
    return address;
}

template <typename T>
T Device::read_config(uint16 offset) const {
    return *reinterpret_cast<volatile T *>(address_for(offset));
}

template <typename T>
void Device::write_config(uint16 offset, T value) {
    *reinterpret_cast<volatile T *>(address_for(offset)) = value;
}

template uint8 Device::read_config(uint16) const;
template uint16 Device::read_config(uint16) const;
template uint32 Device::read_config(uint16) const;

template void Device::write_config(uint16, uint8);
template void Device::write_config(uint16, uint16);
template void Device::write_config(uint16, uint32);

void Device::enable() {
    auto command = read_config<uint16>(4);
    command &= ~(1u << 0u);  // Disable I/O space
    command |= (1u << 1u);   // Enable memory space
    command |= (1u << 2u);   // Enable bus mastering
    command &= ~(1u << 10u); // Enable interrupts
    write_config(4, command);
}

uintptr Device::read_bar(uint8 index) const {
    auto bar = static_cast<uintptr>(read_config<uint32>(0x10 + index * sizeof(uint32)));
    ENSURE((bar & 1u) == 0, "Port I/O not supported");
    if (((bar >> 1u) & 0b11u) == 2) {
        // BAR is a 64-bit address.
        auto bar_ext = read_config<uint32>(0x10 + (index + 1) * sizeof(uint32));
        bar |= static_cast<uintptr>(bar_ext) << 32u;
    }
    return bar & ~0xful;
}

uint16 Device::vendor_id() const {
    return read_config<uint16>(0);
}

uint16 Device::device_id() const {
    return read_config<uint16>(2);
}

uint32 Device::class_info() const {
    return read_config<uint32>(8);
}

} // namespace pci
