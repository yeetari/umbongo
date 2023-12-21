#pragma once

namespace kernel::acpi {

class PciTable;

} // namespace kernel::acpi

namespace kernel::pci {

void enumerate(acpi::PciTable *mcfg);

} // namespace kernel::pci
