#pragma once

#include <ustd/Types.hh>

namespace acpi {

struct InterruptController {
    uint8 type;
    uint8 length;
};

struct IoApicController : public InterruptController {
    uint8 id;
    uint8 : 8;
    uint32 address;
    uint32 gsi_base;
};

struct InterruptSourceOverride : public InterruptController {
    uint8 bus;
    uint8 isa;
    uint32 gsi;
    struct {
        uint8 polarity : 2;
        uint8 trigger_mode : 2;
        uint16 : 12;
    };
};

} // namespace acpi
