#include <kernel/intr/InterruptManager.hh>

#include <kernel/Dmesg.hh>
#include <kernel/Port.hh>
#include <kernel/cpu/Processor.hh>
#include <kernel/cpu/RegisterState.hh>
#include <kernel/intr/InterruptType.hh>
#include <kernel/intr/IoApic.hh>
#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/Function.hh>
#include <ustd/Optional.hh>
#include <ustd/Try.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>

namespace kernel {
namespace {

struct Override {
    uint8_t isa;
    uint32_t gsi;
    InterruptPolarity polarity;
    InterruptTriggerMode trigger_mode;
};

// TODO: Make these Vectors with inline capacity.
ustd::Array<IoApic, 4> s_io_apics;
ustd::Array<Override, 8> s_overrides;
ustd::Array<ustd::Function<void()>, 256> s_custom_handlers;
uint8_t s_current_vector = 70;
uint8_t s_io_apic_count = 0;
uint8_t s_override_count = 0;

const char *polarity_str(InterruptPolarity polarity) {
    switch (polarity) {
    case InterruptPolarity::ActiveHigh:
        return "active-high";
    case InterruptPolarity::ActiveLow:
        return "active-low";
    }
}

const char *trigger_mode_str(InterruptTriggerMode trigger_mode) {
    switch (trigger_mode) {
    case InterruptTriggerMode::EdgeTriggered:
        return "edge-triggered";
    case InterruptTriggerMode::LevelTriggered:
        return "level-triggered";
    }
}

ustd::Optional<IoApic &> io_apic_for_gsi(uint32_t gsi) {
    for (size_t i = 0; i < s_io_apic_count; i++) {
        auto &io_apic = s_io_apics[i];
        if (gsi >= io_apic.gsi_base() && gsi < io_apic.gsi_base() + io_apic.redirection_entry_count()) {
            return io_apic;
        }
    }
    return {};
}

} // namespace

// TODO: Proper allocator.
uint8_t InterruptManager::allocate(ustd::Function<void()> &&handler) {
    uint8_t vector = s_current_vector++;
    s_custom_handlers[vector] = ustd::move(handler);
    Processor::wire_interrupt(vector, [](RegisterState *regs) {
        s_custom_handlers[regs->int_num]();
    });
    return vector;
}

void InterruptManager::mask_pic() {
    dmesg("intr: Found legacy PIC, masking");
    port_write<uint8_t>(0x21, 0xff);
    port_write<uint8_t>(0xa1, 0xff);
}

void InterruptManager::register_io_apic(uint32_t base, uint32_t gsi_base) {
    ENSURE(s_io_apic_count < s_io_apics.size());
    auto *io_apic = new (&s_io_apics[s_io_apic_count++]) IoApic(reinterpret_cast<volatile uint32_t *>(base), gsi_base);
    dmesg("intr: Found IO APIC {} at {} controlling interrupts {} to {}", io_apic->id(), io_apic->base(), gsi_base,
          gsi_base + io_apic->redirection_entry_count());
}

void InterruptManager::register_override(uint8_t isa, uint32_t gsi, InterruptPolarity polarity,
                                         InterruptTriggerMode trigger_mode) {
    ENSURE(s_override_count < s_overrides.size());
    dmesg("intr: Redirecting ISA {} to GSI {} ({}, {})", isa, gsi, polarity_str(polarity),
          trigger_mode_str(trigger_mode));
    s_overrides[s_override_count++] = {isa, gsi, polarity, trigger_mode};
}

void InterruptManager::wire_interrupt(uint32_t irq, uint8_t vector) {
    InterruptPolarity polarity = InterruptPolarity::ActiveHigh;
    InterruptTriggerMode trigger_mode = InterruptTriggerMode::EdgeTriggered;
    for (size_t i = 0; i < s_override_count; i++) {
        auto &override = s_overrides[i];
        if (override.isa == irq) {
            irq = override.gsi;
            polarity = override.polarity;
            trigger_mode = override.trigger_mode;
        }
    }
    IoApic &io_apic = EXPECT(io_apic_for_gsi(irq));
    io_apic.set_redirection_entry(irq, vector, polarity, trigger_mode);
}

} // namespace kernel
