#pragma once

#include <kernel/acpi/InterruptController.hh>
#include <kernel/acpi/Table.hh>
#include <ustd/Array.hh>
#include <ustd/Types.hh>

namespace acpi {

class ApicTableIterator;

// Multiple APIC Description Table (ACPI specification 6.4 section 5.2.8)
class [[gnu::packed]] ApicTable : public Table {
    uint32 m_local_apic;
    uint32 m_flags;
    ustd::Array<InterruptController, 0> m_controllers;

public:
    static constexpr ustd::Array<uint8, 4> k_signature{'A', 'P', 'I', 'C'};

    ApicTableIterator begin() const;
    ApicTableIterator end() const;

    InterruptController *controller(usize index) const;
    usize controller_count() const;

    void *local_apic() const { return reinterpret_cast<void *>(m_local_apic); }
    uint32 flags() const { return m_flags; }
};

class ApicTableIterator {
    const ApicTable *const m_table;
    usize m_index;

public:
    constexpr ApicTableIterator(const ApicTable *table, usize index) : m_table(table), m_index(index) {}

    constexpr ApicTableIterator &operator++() {
        m_index++;
        return *this;
    }

    constexpr ApicTableIterator &operator--() {
        m_index--;
        return *this;
    }

    constexpr bool operator<=>(const ApicTableIterator &) const = default;
    constexpr InterruptController *operator*() const { return m_table->controller(m_index); }
};

inline ApicTableIterator ApicTable::begin() const {
    return {this, 0};
}

inline ApicTableIterator ApicTable::end() const {
    return {this, controller_count()};
}

} // namespace acpi
