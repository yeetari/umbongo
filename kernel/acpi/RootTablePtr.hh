#pragma once

#include <ustd/Array.hh>
#include <ustd/Types.hh>

namespace acpi {

class RootTable;

// Root System Descriptor Pointer (ACPI specification 6.4 section 5.2.5.3)
class [[gnu::packed]] RootTablePtr {
    ustd::Array<uint8, 8> m_signature;
    uint8 m_checksum;
    ustd::Array<uint8, 6> m_oem_id;
    uint8 m_revision;
    uint32 : 32;
    uint32 m_length;
    RootTable *m_xsdt;
    uint8 m_extended_checksum;
    usize : 24;

public:
    uint8 revision() const { return m_revision; }
    RootTable *xsdt() const { return m_xsdt; }
};

} // namespace acpi
