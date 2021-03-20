#include <kernel/cpu/LocalApic.hh>

#include <ustd/Types.hh>

namespace {

constexpr usize k_reg_eoi = 0xb0;
constexpr usize k_reg_siv = 0xf0;

constexpr usize k_reg_timer = 0x320;
constexpr usize k_reg_timer_count = 0x380;
constexpr usize k_reg_timer_count_current = 0x390;
constexpr usize k_reg_timer_divisor = 0x3e0;

} // namespace

template <>
uint32 LocalApic::read(usize reg) const {
    return *(reinterpret_cast<const volatile uint32 *>(this + reg));
}

template <>
void LocalApic::write(usize reg, uint32 val) {
    *(reinterpret_cast<volatile uint32 *>(this + reg)) = val;
}

void LocalApic::enable() {
    // Set spurious interrupt vector to 255 and set bit 8 to software enable the APIC.
    write(k_reg_siv, 255u | (1u << 8u));
}

void LocalApic::send_eoi() {
    write(k_reg_eoi, 0u);
}

void LocalApic::set_timer(TimerMode mode, uint8 vector) {
    // Set timer divisor to 16.
    write(k_reg_timer_divisor, 0b11u);
    write(k_reg_timer, vector | (static_cast<uint32>(mode) << 16u));
}

void LocalApic::set_timer_count(uint32 count) {
    write(k_reg_timer_count, count);
}

uint32 LocalApic::read_timer_count() const {
    return read<uint32>(k_reg_timer_count_current);
}
