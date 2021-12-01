#pragma once

#include <kernel/acpi/GenericAddress.hh>
#include <kernel/acpi/Table.hh>
#include <ustd/Array.hh>
#include <ustd/Types.hh>

namespace acpi {

// High Precision Event Timer Table (IA-PC HPET specification 1.0a section 3.2.4)
class [[gnu::packed]] HpetTable : public Table {
    uint8 m_hardware_revision;
    uint8 m_comparator_count : 5;
    uint8 m_counter_size : 1;
    usize : 1;
    bool m_legacy_replacement_capable : 1;
    uint16 m_pci_vendor_id;
    GenericAddress m_base_address;
    uint8 m_number;
    uint16 m_minimum_tick;
    uint8 m_page_protection;

public:
    static constexpr ustd::Array<uint8, 4> k_signature{'H', 'P', 'E', 'T'};

    uint8 comparator_count() const { return m_comparator_count; }
    const GenericAddress &base_address() const { return m_base_address; }
};

} // namespace acpi
