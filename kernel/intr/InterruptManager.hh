#pragma once

#include <kernel/intr/InterruptType.hh>
#include <ustd/Types.hh>

struct InterruptManager {
    static void register_io_apic(uint32 base, uint32 gsi_base);
    static void register_override(uint8 isa, uint32 gsi, InterruptPolarity polarity, InterruptTriggerMode trigger_mode);
    static void wire_interrupt(uint32 irq, uint8 vector);
};
