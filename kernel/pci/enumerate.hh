#pragma once

namespace kernel::acpi {

class PciTable;

} // namespace kernel::acpi

namespace kernel::pci {

void enumerate(const acpi::PciTable *mcfg);

} // namespace kernel::pci
