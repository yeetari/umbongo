#pragma once

#include <ustd/array.hh>
#include <ustd/types.hh>

namespace kernel::acpi {

// System Description Table Header (ACPI specification 6.4 section 5.2.6)
class [[gnu::packed]] Table {
    ustd::Array<uint8_t, 4> m_signature;
    uint32_t m_length;
    uint8_t m_revision;
    uint8_t m_checksum;
    ustd::Array<uint8_t, 6> m_oem_id;
    ustd::Array<uint8_t, 8> m_oem_table_id;
    uint32_t m_oem_revision;
    uint32_t m_creator_id;
    uint32_t m_creator_revision;

public:
    Table(const Table &) = delete;
    Table(Table &&) = delete;
    constexpr ~Table() = default;

    Table &operator=(const Table &) = delete;
    Table &operator=(Table &&) = delete;

    bool valid() const;

    const ustd::Array<uint8_t, 4> &signature() const { return m_signature; }
    uint8_t revision() const { return m_revision; }
    uint32_t length() const { return m_length; }
};

} // namespace kernel::acpi
