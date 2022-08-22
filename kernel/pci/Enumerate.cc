#include <kernel/pci/Enumerate.hh>

#include <kernel/acpi/PciTable.hh>
#include <kernel/pci/Function.hh>
#include <ustd/Types.hh>

namespace kernel::pci {
namespace {

uint16 read_vendor_id(const acpi::PciSegment *segment, uint8 bus, uint8 device, uint8 function) {
    uintptr address = segment->base;
    address += static_cast<uintptr>(bus) << 20ul;
    address += static_cast<uintptr>(device) << 15ul;
    address += static_cast<uintptr>(function) << 12ul;
    return *reinterpret_cast<volatile uint16 *>(address);
}

} // namespace

void enumerate(acpi::PciTable *mcfg) {
    for (const auto *segment : *mcfg) {
        const uint8 bus_count = 1;
        for (uint8 bus = 0; bus < bus_count; bus++) {
            for (uint8 device = 0; device < 32; device++) {
                for (uint8 function = 0; function < 8; function++) {
                    if (read_vendor_id(segment, bus, device, function) != 0xffffu) {
                        (new Function(segment->base, segment->num, bus, device, function))->leak_ref();
                    }
                }
            }
        }
    }
}

} // namespace kernel::pci
