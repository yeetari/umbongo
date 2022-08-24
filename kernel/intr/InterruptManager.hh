#pragma once

#include <kernel/intr/InterruptType.hh>
#include <ustd/Function.hh> // IWYU pragma: keep
#include <ustd/Types.hh>

namespace kernel {

struct InterruptManager {
    static uint8_t allocate(ustd::Function<void()> &&handler);
    static void mask_pic();
    static void register_io_apic(uint32_t base, uint32_t gsi_base);
    static void register_override(uint8_t isa, uint32_t gsi, InterruptPolarity polarity,
                                  InterruptTriggerMode trigger_mode);
    static void wire_interrupt(uint32_t irq, uint8_t vector);
};

} // namespace kernel
