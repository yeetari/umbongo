#pragma once

#include <ustd/types.hh>

namespace kernel::acpi {

struct InterruptController {
    uint8_t type;
    uint8_t length;
};

struct [[gnu::packed]] LocalApicController : public InterruptController {
    uint8_t acpi_id;
    uint8_t apic_id;
    uint32_t flags;
};

struct [[gnu::packed]] IoApicController : public InterruptController {
    uint8_t id;
    uint8_t : 8;
    uint32_t address;
    uint32_t gsi_base;
};

struct [[gnu::packed]] InterruptSourceOverride : public InterruptController {
    uint8_t bus;
    uint8_t isa;
    uint32_t gsi;
    struct {
        uint8_t polarity : 2;
        uint8_t trigger_mode : 2;
        uint16_t : 12;
    };
};

} // namespace kernel::acpi
