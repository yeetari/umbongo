#pragma once

#include <kernel/acpi/Table.hh>
#include <ustd/Array.hh>
#include <ustd/Types.hh>

namespace acpi {

class PciTableIterator;

struct [[gnu::packed]] PciSegment {
    uint64 base;
    uint16 num;
    uint8 start_bus;
    uint8 end_bus;
    uint32 : 32;
};

class [[gnu::packed]] PciTable : public Table {
    uint64 : 64;
    Array<PciSegment, 0> m_segments;

public:
    static constexpr Array<uint8, 4> k_signature{'M', 'C', 'F', 'G'};

    PciTableIterator begin() const;
    PciTableIterator end() const;

    const PciSegment *segment(usize index) const;
    usize segment_count() const;
};

class PciTableIterator {
    const PciTable *const m_table;
    usize m_index;

public:
    constexpr PciTableIterator(const PciTable *table, usize index) : m_table(table), m_index(index) {}

    constexpr PciTableIterator &operator++() {
        m_index++;
        return *this;
    }

    constexpr PciTableIterator &operator--() {
        m_index--;
        return *this;
    }

    constexpr bool operator<=>(const PciTableIterator &) const = default;
    constexpr const PciSegment *operator*() const { return m_table->segment(m_index); }
};

inline PciTableIterator PciTable::begin() const {
    return {this, 0};
}

inline PciTableIterator PciTable::end() const {
    return {this, segment_count()};
}

} // namespace acpi
