#pragma once

#include <ustd/function.hh> // IWYU pragma: keep
#include <ustd/types.hh>
// IWYU pragma: no_forward_declare ustd::Function

namespace kernel {

enum class InterruptPolarity;
enum class InterruptTriggerMode;

struct InterruptManager {
    static uint8_t allocate(ustd::Function<void()> &&handler);
    static void register_io_apic(uint32_t base, uint32_t gsi_base);
    static void register_override(uint8_t isa, uint32_t gsi, InterruptPolarity polarity,
                                  InterruptTriggerMode trigger_mode);
    static void wire_interrupt(uint32_t irq, uint8_t vector);
};

} // namespace kernel
