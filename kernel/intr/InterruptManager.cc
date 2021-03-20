#include <kernel/intr/InterruptManager.hh>

#include <kernel/intr/IoApic.hh>
#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/Log.hh>
#include <ustd/Memory.hh>
#include <ustd/Types.hh>

namespace {

struct Override {
    uint8 isa;
    uint32 gsi;
    InterruptPolarity polarity;
    InterruptTriggerMode trigger_mode;
};

// TODO: Make these Vectors with inline capacity.
Array<IoApic, 4> s_io_apics;
Array<Override, 8> s_overrides;
uint8 s_io_apic_count = 0;
uint8 s_override_count = 0;

const char *polarity_str(InterruptPolarity polarity) {
    switch (polarity) {
    case InterruptPolarity::ActiveHigh:
        return "active-high";
    case InterruptPolarity::ActiveLow:
        return "active-low";
    default:
        ENSURE_NOT_REACHED();
    }
}

const char *trigger_mode_str(InterruptTriggerMode trigger_mode) {
    switch (trigger_mode) {
    case InterruptTriggerMode::EdgeTriggered:
        return "edge-triggered";
    case InterruptTriggerMode::LevelTriggered:
        return "level-triggered";
    default:
        ENSURE_NOT_REACHED();
    }
}

IoApic *io_apic_for_gsi(uint32 gsi) {
    for (usize i = 0; i < s_io_apic_count; i++) {
        auto &io_apic = s_io_apics[i];
        if (gsi >= io_apic.gsi_base() && gsi < io_apic.gsi_base() + io_apic.redirection_entry_count()) {
            return &io_apic;
        }
    }
    return nullptr;
}

} // namespace

void InterruptManager::register_io_apic(uint32 base, uint32 gsi_base) {
    ENSURE(s_io_apic_count < s_io_apics.size());
    auto *io_apic = new (&s_io_apics[s_io_apic_count++]) IoApic(reinterpret_cast<volatile uint32 *>(base), gsi_base);
    logln("intr: Found IO APIC {} at {} controlling interrupts {} to {}", io_apic->id(), io_apic->base(), gsi_base,
          gsi_base + io_apic->redirection_entry_count());
}

void InterruptManager::register_override(uint8 isa, uint32 gsi, InterruptPolarity polarity,
                                         InterruptTriggerMode trigger_mode) {
    ENSURE(s_override_count < s_overrides.size());
    logln("intr: Redirecting ISA {} to GSI {} ({}, {})", isa, gsi, polarity_str(polarity),
          trigger_mode_str(trigger_mode));
    s_overrides[s_override_count++] = {isa, gsi, polarity, trigger_mode};
}

void InterruptManager::wire_interrupt(uint32 irq, uint8 vector) {
    InterruptPolarity polarity = InterruptPolarity::ActiveHigh;
    InterruptTriggerMode trigger_mode = InterruptTriggerMode::EdgeTriggered;
    for (usize i = 0; i < s_override_count; i++) {
        auto &override = s_overrides[i];
        if (override.isa == irq) {
            irq = override.gsi;
            polarity = override.polarity;
            trigger_mode = override.trigger_mode;
        }
    }
    auto *io_apic = io_apic_for_gsi(irq);
    ENSURE(io_apic != nullptr);
    io_apic->set_redirection_entry(irq, vector, polarity, trigger_mode);
}