#include <kernel/acpi/pci_table.hh>

#include <kernel/acpi/table.hh>
#include <ustd/array.hh>
#include <ustd/assert.hh>
#include <ustd/types.hh>

namespace kernel::acpi {

const PciSegment *PciTable::segment(size_t index) const {
    ASSERT(index < segment_count());
    return &m_segments.data()[index];
}

size_t PciTable::segment_count() const {
    return (length() - sizeof(Table) - sizeof(uint64_t)) / sizeof(PciSegment);
}

} // namespace kernel::acpi
