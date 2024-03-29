#include <kernel/pci/function.hh>

#include <kernel/api/types.h>
#include <kernel/dev/device.hh>
#include <kernel/error.hh>
#include <kernel/intr/interrupt_manager.hh>
#include <kernel/mem/address_space.hh>
#include <kernel/mem/region.hh>
#include <kernel/mem/vm_object.hh>
#include <kernel/sys_result.hh>
#include <ustd/array.hh>
#include <ustd/assert.hh>
#include <ustd/numeric.hh>
#include <ustd/scoped_change.hh>
#include <ustd/span.hh>
#include <ustd/string_builder.hh>
#include <ustd/try.hh>
#include <ustd/types.hh>
#include <ustd/utility.hh>

namespace kernel::pci {
namespace {

constexpr uint8_t k_cap_msi = 0x05;
constexpr uint8_t k_cap_msix = 0x11;
constexpr uint32_t k_reg_vendor_id = 0x00;
constexpr uint32_t k_reg_device_id = 0x02;
constexpr uint32_t k_reg_command = 0x04;
constexpr uint32_t k_reg_status = 0x06;
constexpr uint32_t k_reg_class_info = 0x08;

} // namespace

Function::Function(uintptr_t segment_base, uint16_t segment, uint8_t bus, uint8_t device, uint8_t function)
    : Device(ustd::format("pci/{:x4}:{:x2}:{:x2}.{}", segment, bus, device, function)), m_segment(segment), m_bus(bus),
      m_device(device), m_function(function), m_address(segment_base) {
    m_address += static_cast<uintptr_t>(bus) << 20ul;
    m_address += static_cast<uintptr_t>(device) << 15ul;
    m_address += static_cast<uintptr_t>(function) << 12ul;

    // Disable device and read identifying information.
    const auto command_register = read_config<uint16_t>(k_reg_command);
    write_config<uint16_t>(k_reg_command, 0);
    m_vendor_id = read_config<uint16_t>(k_reg_vendor_id);
    m_device_id = read_config<uint16_t>(k_reg_device_id);
    m_class_info = read_config<uint32_t>(k_reg_class_info);

    // Read BARs.
    // TODO: Not all device types have 6 BARs.
    for (uint8_t i = 0; i < 6; i++) {
        auto &bar_lower = *reinterpret_cast<volatile uint32_t *>(m_address + 0x10 + i * sizeof(uint32_t));
        const bool wide = ((bar_lower >> 1u) & 0b11u) == 0b10u;
        m_bars[i].address = bar_lower;
        {
            ustd::ScopedChange change(bar_lower, 0xffffffffu);
            m_bars[i].size = bar_lower;
        }
        if (wide) {
            auto &bar_upper = *reinterpret_cast<volatile uint32_t *>(m_address + 0x10 + (i + 1) * sizeof(uint32_t));
            m_bars[i].address |= static_cast<uintptr_t>(bar_upper) << 32u;
            {
                ustd::ScopedChange change(bar_upper, 0xffffffffu);
                m_bars[i].size |= static_cast<size_t>(bar_upper) << 32u;
            }
        }
        m_bars[i].address &= ~0xful;
        m_bars[i].size = ~(m_bars[i].size & ~0xful) + 1u;
        if (!wide) {
            m_bars[i].size &= 0xffffffffu;
        }
    }

    // Re-enable memory and/or io space.
    write_config(k_reg_command, command_register & 0b11u);
}

template <typename F>
void Function::enumerate_capabilities(F callback) const {
    if ((read_config<uint16_t>(k_reg_status) & (1u << 4u)) == 0u) {
        // Capabilities list not present.
        return;
    }
    auto cap_ptr = read_config<uint32_t>(52) & 0xffu;
    while (cap_ptr != 0u) {
        auto cap_reg = read_config<uint32_t>(cap_ptr);
        if (!callback(cap_ptr, cap_reg)) {
            break;
        }
        cap_ptr = (cap_reg >> 8u) & 0xffu;
    }
}

template <typename T>
T Function::read_config(uint32_t offset) const {
    return *reinterpret_cast<volatile T *>(m_address + offset);
}

template <typename T>
void Function::write_config(uint32_t offset, T value) {
    *reinterpret_cast<volatile T *>(m_address + offset) = value;
}

SyscallResult Function::ioctl(ub_ioctl_request_t request, void *) {
    switch (request) {
    case UB_IOCTL_REQUEST_PCI_ENABLE_DEVICE:
        // Enable device by enabling memory space, bus mastering, and disabling legacy interrupts.
        write_config<uint16_t>(k_reg_command, (1u << 10u) | (1u << 2u) | (1u << 1u));
        return 0;
    case UB_IOCTL_REQUEST_PCI_ENABLE_INTERRUPTS: {
        auto irq_handler = [this] {
            m_interrupt_pending = true;
        };
        enumerate_capabilities([this, &irq_handler](uint32_t cap_ptr, uint32_t cap_reg) {
            switch (cap_reg & 0xffu) {
            case k_cap_msi:
                // Enable MSI and ensure 64-bit is available.
                write_config(cap_ptr, read_config<uint32_t>(cap_ptr) | (1u << 16u));
                ENSURE((read_config<uint32_t>(cap_ptr) & (1u << 23u)) != 0u);

                // Write message address (low then high), then message data, then clear the mask.
                write_config<uint32_t>(cap_ptr + 4u, 0xfee00000u);
                write_config<uint32_t>(cap_ptr + 8u, 0u);
                write_config<uint32_t>(cap_ptr + 12u,
                                       (1u << 14u) | InterruptManager::allocate(ustd::move(irq_handler)));
                write_config<uint32_t>(cap_ptr + 16u, 0u);
                return false;
            case k_cap_msix: {
                // Enable MSI-X and unmask the whole function.
                write_config(cap_ptr, (read_config<uint32_t>(cap_ptr) | (1u << 31u)) & ~(1u << 30u));

                // Retrieve the base index register (BIR) from the second capability register, then retrieve the BAR
                // itself.
                uintptr_t bar = m_bars[read_config<uint32_t>(cap_ptr + 4u) & 0b111u].address;
                ENSURE(bar != 0u);

                // Retrieve the table length and offset from the capability registers. The table length is encoded as
                // the lower 11 bits in the message control word, which itself is located in the upper 16 bits of the
                // first capability register. The table offset is stored in the second capability register, but it is
                // always 8-byte aligned to allow the BIR to be placed in the lower 3 bits, so we mask those bits off.
                uint16_t table_length = ((read_config<uint32_t>(cap_ptr) >> 16u) & 0x7ffu) + 1u;
                uint32_t table_offset = read_config<uint32_t>(cap_ptr + 4u) & ~0b111u;
                ENSURE(table_length >= 1u && table_length <= 2048u);

                // Fill table entry.
                auto *table = reinterpret_cast<volatile uint32_t *>(bar + table_offset);
                for (uint16_t i = 0; i < table_length; i++) {
                    table[i * 4u + 0u] = 0xfee00000u;
                    table[i * 4u + 1u] = 0u;
                    table[i * 4u + 2u] = (1u << 14u) | InterruptManager::allocate(ustd::move(irq_handler));
                    table[i * 4u + 3u] = 0u;
                }
                return false;
            }
            }
            return true;
        });
        return 0;
    }
    default:
        return Error::Invalid;
    }
}

uintptr_t Function::mmap(AddressSpace &address_space) const {
    constexpr auto access = RegionAccess::Writable | RegionAccess::UserAccessible | RegionAccess::Uncacheable;
    const auto &bar = m_bars[0];
    auto vm_object = VmObject::create_physical(bar.address, bar.size);
    auto &region = EXPECT(address_space.allocate_anywhere(bar.size, access));
    region.map(ustd::move(vm_object));
    return region.base();
}

SysResult<size_t> Function::read(ustd::Span<void> data, size_t offset) {
    m_interrupt_pending = false;
    if (data.data() == nullptr) {
        return 0u;
    }
    if (offset != 0) {
        return 0u;
    }
    ub_pci_device_info_t info{
        .bars{
            ub_pci_bar_t{m_bars[0].address, m_bars[0].size},
            ub_pci_bar_t{m_bars[1].address, m_bars[1].size},
            ub_pci_bar_t{m_bars[2].address, m_bars[2].size},
            ub_pci_bar_t{m_bars[3].address, m_bars[3].size},
            ub_pci_bar_t{m_bars[4].address, m_bars[4].size},
            ub_pci_bar_t{m_bars[5].address, m_bars[5].size},
        },
        .vendor_id = m_vendor_id,
        .device_id = m_device_id,
        .clas = static_cast<uint8_t>((m_class_info >> 24u) & 0xffu),
        .subc = static_cast<uint8_t>((m_class_info >> 16u) & 0xffu),
        .prif = static_cast<uint8_t>((m_class_info >> 8u) & 0xffu),
    };
    const auto size = ustd::min(data.size(), sizeof(ub_pci_device_info_t));
    __builtin_memcpy(data.data(), &info, size);
    return size;
}

} // namespace kernel::pci
