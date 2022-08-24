#pragma once

#include <kernel/acpi/Table.hh>
#include <ustd/Array.hh>
#include <ustd/Types.hh>

namespace kernel::acpi {

class PciTableIterator;

struct [[gnu::packed]] PciSegment {
    uint64_t base;
    uint16_t num;
    uint8_t start_bus;
    uint8_t end_bus;
    uint32_t : 32;
};

class [[gnu::packed]] PciTable : public Table {
    uint64_t : 64;
    ustd::Array<PciSegment, 0> m_segments;

public:
    static constexpr ustd::Array<uint8_t, 4> k_signature{'M', 'C', 'F', 'G'};

    PciTableIterator begin() const;
    PciTableIterator end() const;

    const PciSegment *segment(size_t index) const;
    size_t segment_count() const;
};

class PciTableIterator {
    const PciTable *const m_table;
    size_t m_index;

public:
    constexpr PciTableIterator(const PciTable *table, size_t index) : m_table(table), m_index(index) {}

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

} // namespace kernel::acpi
