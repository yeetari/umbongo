#include <kernel/acpi/PciTable.hh>

#include <kernel/acpi/Table.hh>
#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/Types.hh>

namespace kernel::acpi {

const PciSegment *PciTable::segment(size_t index) const {
    ASSERT(index < segment_count());
    return &m_segments.data()[index];
}

size_t PciTable::segment_count() const {
    return (length() - sizeof(Table) - sizeof(uint64_t)) / sizeof(PciSegment);
}

} // namespace kernel::acpi
