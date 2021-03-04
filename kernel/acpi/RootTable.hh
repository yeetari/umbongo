#pragma once

#include <kernel/acpi/Table.hh>
#include <ustd/Array.hh>

namespace acpi {

class RootTableIterator;

// Extended System Description Table (ACPI specification 6.4 section 5.2.8)
class [[gnu::packed]] RootTable : public Table {
    Array<Table *, 1> m_entries;

public:
    RootTableIterator begin() const;
    RootTableIterator end() const;

    Table *entry(usize index) const;
    usize entry_count() const;
};

class RootTableIterator {
    const RootTable *const m_table;
    usize m_index;

public:
    constexpr RootTableIterator(const RootTable *table, usize index) : m_table(table), m_index(index) {}

    constexpr RootTableIterator &operator++() {
        m_index++;
        return *this;
    }

    constexpr RootTableIterator &operator--() {
        m_index--;
        return *this;
    }

    constexpr bool operator<=>(const RootTableIterator &) const = default;
    constexpr Table *operator*() const { return m_table->entry(m_index); }
};

inline RootTableIterator RootTable::begin() const {
    return {this, 0};
}

inline RootTableIterator RootTable::end() const {
    return {this, entry_count()};
}

} // namespace acpi
