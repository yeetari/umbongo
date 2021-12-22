#pragma once

namespace acpi {

class PciTable;

} // namespace acpi

namespace pci {

void enumerate(acpi::PciTable *mcfg);

} // namespace pci
