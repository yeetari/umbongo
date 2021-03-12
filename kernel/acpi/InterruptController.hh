#pragma once

#include <ustd/Types.hh>

namespace acpi {

class [[gnu::packed]] InterruptController {
    uint8 m_type;
    uint8 m_length;

public:
    uint8 type() const { return m_type; }
    uint8 length() const { return m_length; }
};

struct [[gnu::packed]] IoApicController : public InterruptController {
    uint8 m_id;
    uint8 : 8;
    uint32 m_address;
    uint32 m_interrupt_base;
};

struct [[gnu::packed]] InterruptSourceOverride : public InterruptController {
    uint8 m_bus;
    uint8 m_source;
    uint32 m_global_system_interrupt;
    uint16 m_flags;
};

} // namespace acpi
