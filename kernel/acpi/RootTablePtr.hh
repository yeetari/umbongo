#pragma once

#include <ustd/Array.hh>
#include <ustd/Types.hh>

namespace kernel::acpi {

class RootTable;

// Root System Descriptor Pointer (ACPI specification 6.4 section 5.2.5.3)
class [[gnu::packed]] RootTablePtr {
    ustd::Array<uint8_t, 8> m_signature;
    uint8_t m_checksum;
    ustd::Array<uint8_t, 6> m_oem_id;
    uint8_t m_revision;
    uint32_t : 32;
    uint32_t m_length;
    RootTable *m_xsdt;
    uint8_t m_extended_checksum;
    size_t : 24;

public:
    uint8_t revision() const { return m_revision; }
    RootTable *xsdt() const { return m_xsdt; }
};

} // namespace kernel::acpi
