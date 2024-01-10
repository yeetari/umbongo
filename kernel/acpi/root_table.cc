#include <kernel/acpi/root_table.hh>

#include <kernel/acpi/table.hh>
#include <ustd/assert.hh>
#include <ustd/types.hh>

namespace kernel::acpi {

const Table *RootTable::entry(size_t index) const {
    ASSERT(index < entry_count());
    return m_entries[index];
}

size_t RootTable::entry_count() const {
    return (length() - sizeof(Table)) / sizeof(void *);
}

} // namespace kernel::acpi
