#include <kernel/acpi/PciTable.hh>

#include <kernel/acpi/Table.hh>
#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/Types.hh>

namespace acpi {

const PciSegment *PciTable::segment(usize index) const {
    ASSERT(index < segment_count());
    return &m_segments.data()[index];
}

usize PciTable::segment_count() const {
    return (length() - sizeof(Table) - sizeof(uint64)) / sizeof(PciSegment);
}

} // namespace acpi
