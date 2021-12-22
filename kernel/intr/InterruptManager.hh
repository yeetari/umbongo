#pragma once

#include <kernel/intr/InterruptType.hh>
#include <ustd/Function.hh> // IWYU pragma: keep
#include <ustd/Types.hh>

struct InterruptManager {
    static uint8 allocate(ustd::Function<void()> &&handler);
    static void mask_pic();
    static void register_io_apic(uint32 base, uint32 gsi_base);
    static void register_override(uint8 isa, uint32 gsi, InterruptPolarity polarity, InterruptTriggerMode trigger_mode);
    static void wire_interrupt(uint32 irq, uint8 vector);
};
