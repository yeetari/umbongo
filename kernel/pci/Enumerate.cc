#include <kernel/pci/Enumerate.hh>

#include <kernel/acpi/PciTable.hh>
#include <kernel/pci/Function.hh>
#include <ustd/Types.hh>

namespace kernel::pci {
namespace {

uint16_t read_vendor_id(const acpi::PciSegment *segment, uint8_t bus, uint8_t device, uint8_t function) {
    uintptr_t address = segment->base;
    address += static_cast<uintptr_t>(bus) << 20ul;
    address += static_cast<uintptr_t>(device) << 15ul;
    address += static_cast<uintptr_t>(function) << 12ul;
    return *reinterpret_cast<volatile uint16_t *>(address);
}

} // namespace

void enumerate(acpi::PciTable *mcfg) {
    for (const auto *segment : *mcfg) {
        const uint8_t bus_count = segment->end_bus - segment->start_bus;
        for (uint8_t bus = 0; bus < bus_count; bus++) {
            for (uint8_t device = 0; device < 32; device++) {
                for (uint8_t function = 0; function < 8; function++) {
                    if (read_vendor_id(segment, bus, device, function) != 0xffffu) {
                        (new Function(segment->base, segment->num, bus, device, function))->leak_ref();
                    }
                }
            }
        }
    }
}

} // namespace kernel::pci
