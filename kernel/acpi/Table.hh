#pragma once

#include <ustd/Array.hh>
#include <ustd/Types.hh>

namespace acpi {

// System Description Table Header (ACPI specification 6.4 section 5.2.6)
class [[gnu::packed]] Table {
    ustd::Array<uint8, 4> m_signature;
    uint32 m_length;
    uint8 m_revision;
    uint8 m_checksum;
    ustd::Array<uint8, 6> m_oem_id;
    ustd::Array<uint8, 8> m_oem_table_id;
    uint32 m_oem_revision;
    uint32 m_creator_id;
    uint32 m_creator_revision;

public:
    Table(const Table &) = delete;
    Table(Table &&) = delete;
    constexpr ~Table() = default;

    Table &operator=(const Table &) = delete;
    Table &operator=(Table &&) = delete;

    bool valid() const;

    const ustd::Array<uint8, 4> &signature() const { return m_signature; }
    uint8 revision() const { return m_revision; }
    uint32 length() const { return m_length; }
};

} // namespace acpi
