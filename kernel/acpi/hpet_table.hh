#pragma once

#include <kernel/acpi/generic_address.hh>
#include <kernel/acpi/table.hh>
#include <ustd/array.hh>
#include <ustd/types.hh>

namespace kernel::acpi {

// High Precision Event Timer Table (IA-PC HPET specification 1.0a section 3.2.4)
class [[gnu::packed]] HpetTable : public Table {
    uint8_t m_hardware_revision;
    uint8_t m_comparator_count : 5;
    uint8_t m_counter_size : 1;
    size_t : 1;
    bool m_legacy_replacement_capable : 1;
    uint16_t m_pci_vendor_id;
    GenericAddress m_base_address;
    uint8_t m_number;
    uint16_t m_minimum_tick;
    uint8_t m_page_protection;

public:
    static constexpr ustd::Array<uint8_t, 4> k_signature{'H', 'P', 'E', 'T'};

    uint8_t comparator_count() const { return m_comparator_count; }
    const GenericAddress &base_address() const { return m_base_address; }
};

} // namespace kernel::acpi
