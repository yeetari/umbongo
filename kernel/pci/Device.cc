#include <kernel/pci/Device.hh>

#include <kernel/pci/Bus.hh>
#include <ustd/Assert.hh>
#include <ustd/Types.hh>

namespace pci {

uintptr Device::address_for(uint32 offset) const {
    uintptr address = m_bus->segment_base();
    address += static_cast<uintptr>(m_bus->num()) << 20ul;
    address |= static_cast<uintptr>(m_device) << 15ul;
    address |= static_cast<uintptr>(m_function) << 12ul;
    address |= static_cast<uintptr>(offset);
    return address;
}

template <typename T>
T Device::read_config(uint32 offset) const {
    return *reinterpret_cast<volatile T *>(address_for(offset));
}

template <typename T>
void Device::write_config(uint32 offset, T value) {
    *reinterpret_cast<volatile T *>(address_for(offset)) = value;
}

template uint8 Device::read_config(uint32) const;
template uint16 Device::read_config(uint32) const;
template uint32 Device::read_config(uint32) const;

template void Device::write_config(uint32, uint8);
template void Device::write_config(uint32, uint16);
template void Device::write_config(uint32, uint32);

void Device::configure() {
    auto command = read_config<uint16>(4);
    command &= ~(1u << 0u); // Disable I/O space
    command |= (1u << 1u);  // Enable memory space
    command |= (1u << 2u);  // Enable bus mastering
    command |= (1u << 10u); // Disable interrupt pins
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

template <typename F>
void Device::walk_capabilities(F callback) const {
    auto status = read_config<uint32>(4) >> 16u;
    if ((status & (1u << 4u)) == 0) {
        // Capabilities list not present.
        return;
    }

    auto cap_ptr = read_config<uint32>(52) & 0xffu;
    while (cap_ptr != 0) {
        auto cap_reg = read_config<uint32>(cap_ptr);
        callback(cap_ptr, cap_reg);
        cap_ptr = (cap_reg >> 8u) & 0xffu;
    }
}

void Device::enable_msi(uint8 vector) {
    uint32 reg_ptr = 0;
    walk_capabilities([&reg_ptr](uint32 cap_ptr, uint32 cap_reg) {
        if ((cap_reg & 0xffu) == 0x05) {
            reg_ptr = cap_ptr;
        }
    });
    ENSURE(reg_ptr != 0);

    // Enable MSI and ensure 64-bit is available.
    write_config<uint32>(reg_ptr, read_config<uint32>(reg_ptr) | (1u << 16u));
    ENSURE((read_config<uint32>(reg_ptr) & (1u << 23u)) != 0);

    // x86 specific data for the MSI table entry.
    uint64 msi_addr = 0xfee00000;
    uint16 msi_data = (1u << 14u) | vector;

    // Write message address (low then high), then message data, and then make sure the mask is clear.
    write_config<uint32>(reg_ptr + 4, msi_addr & 0xffffffffu);
    write_config<uint32>(reg_ptr + 8, msi_addr >> 32u);
    write_config<uint32>(reg_ptr + 12, msi_data);
    write_config<uint32>(reg_ptr + 16, 0);
}

void Device::enable_msix(uint8 vector) {
    uint32 reg_ptr = 0;
    walk_capabilities([&reg_ptr](uint32 cap_ptr, uint32 cap_reg) {
        if ((cap_reg & 0xffu) == 0x11) {
            reg_ptr = cap_ptr;
        }
    });
    ENSURE(reg_ptr != 0);

    // Enable MSI-X and unmask the whole function.
    write_config<uint32>(reg_ptr, (read_config<uint32>(reg_ptr) | (1u << 31u)) & ~(1u << 30u));

    // Retrieve the base index register (BIR) from the second capability register, and then retrieve the BAR itself.
    uintptr bar = read_bar(read_config<uint32>(reg_ptr + 4) & 0b111u);
    ENSURE(bar != 0);

    // Retrieve the table length and offset from the capability registers. The table length is encoded as the lower 11
    // bits in the message control word, which itself is located in the upper 16 bits of the first capability register.
    // The table offset is stored in the second capability register, but it is always 8-byte aligned to allow the BIR to
    // be placed in the lower 3 bits, so we mask those bits off.
    uint16 table_length = ((read_config<uint32>(reg_ptr) >> 16u) & 0x7ffu) + 1;
    uint32 table_offset = read_config<uint32>(reg_ptr + 4) & ~0b111u;
    ENSURE(table_length >= 1 && table_length <= 2048);

    // x86 specific data for the MSI-X table entry.
    uint64 msix_addr = 0xfee00000;
    uint32 msix_data = (1u << 14u) | vector;

    // Fill table entry.
    auto *table = reinterpret_cast<volatile uint32 *>(bar + table_offset);
    for (uint16 i = 0; i < table_length; i++) {
        table[i * 4 + 0] = msix_addr & 0xffffffffu;
        table[i * 4 + 1] = msix_addr >> 32u;
        table[i * 4 + 2] = msix_data;
        table[i * 4 + 3] = 0;
    }
}

bool Device::msi_supported() const {
    bool supported = false;
    walk_capabilities([&supported](uint32, uint32 cap_reg) {
        if ((cap_reg & 0xffu) == 0x05) {
            supported = true;
        }
    });
    return supported;
}

bool Device::msix_supported() const {
    bool supported = false;
    walk_capabilities([&supported](uint32, uint32 cap_reg) {
        if ((cap_reg & 0xffu) == 0x11) {
            supported = true;
        }
    });
    return supported;
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
