#include <kernel/acpi/RootTable.hh>

#include <kernel/acpi/Table.hh>
#include <ustd/Assert.hh>
#include <ustd/Types.hh>

namespace kernel::acpi {

Table *RootTable::entry(usize index) const {
    ASSERT(index < entry_count());
    return m_entries[index];
}

usize RootTable::entry_count() const {
    return (length() - sizeof(Table)) / sizeof(void *);
}

} // namespace kernel::acpi
