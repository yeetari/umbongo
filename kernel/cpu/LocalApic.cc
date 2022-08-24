#include <kernel/cpu/LocalApic.hh>

#include <ustd/Types.hh>

namespace kernel {
namespace {

constexpr size_t k_reg_eoi = 0xb0;
constexpr size_t k_reg_siv = 0xf0;
constexpr size_t k_reg_icr_low = 0x300;
constexpr size_t k_reg_icr_high = 0x310;

constexpr size_t k_reg_timer = 0x320;
constexpr size_t k_reg_timer_count = 0x380;
constexpr size_t k_reg_timer_count_current = 0x390;
constexpr size_t k_reg_timer_divisor = 0x3e0;

} // namespace

template <>
uint32_t LocalApic::read(size_t reg) const {
    return *(reinterpret_cast<const volatile uint32_t *>(this + reg));
}

template <>
void LocalApic::write(size_t reg, uint32_t val) {
    *(reinterpret_cast<volatile uint32_t *>(this + reg)) = val;
}

void LocalApic::enable() {
    // Set spurious interrupt vector to 255 and set bit 8 to software enable the APIC.
    write(k_reg_siv, 255u | (1u << 8u));
}

void LocalApic::send_eoi() {
    write(k_reg_eoi, 0u);
}

void LocalApic::send_ipi(uint8_t vector, MessageType type, DestinationMode mode, Level level, TriggerMode trigger_mode,
                         DestinationShorthand shorthand, uint8_t destination) {
    write(k_reg_icr_high, static_cast<uint32_t>(destination) << 24u);
    write(k_reg_icr_low, vector | (static_cast<uint32_t>(type) << 8u) | (static_cast<uint32_t>(mode) << 11u) |
                             (static_cast<uint32_t>(level) << 14u) | (static_cast<uint32_t>(trigger_mode) << 15u) |
                             (static_cast<uint32_t>(shorthand) << 18u));
}

void LocalApic::send_ipi(uint8_t vector, MessageType type, DestinationMode mode, Level level, TriggerMode trigger_mode,
                         DestinationShorthand shorthand) {
    send_ipi(vector, type, mode, level, trigger_mode, shorthand, 0);
}

void LocalApic::send_ipi(uint8_t vector, MessageType type, DestinationMode mode, Level level, TriggerMode trigger_mode,
                         uint8_t destination) {
    send_ipi(vector, type, mode, level, trigger_mode, DestinationShorthand::Destination, destination);
}

void LocalApic::set_timer(TimerMode mode, uint8_t vector) {
    // Set timer divisor to 16.
    write(k_reg_timer_divisor, 0b11u);
    write(k_reg_timer, vector | (static_cast<uint32_t>(mode) << 16u));
}

void LocalApic::set_timer_count(uint32_t count) {
    write(k_reg_timer_count, count);
}

uint32_t LocalApic::read_timer_count() const {
    return read<uint32_t>(k_reg_timer_count_current);
}

} // namespace kernel
