#include <kernel/cpu/LocalApic.hh>

#include <ustd/Types.hh>

namespace {

constexpr usize k_reg_tpr = 0x80;
constexpr usize k_reg_eoi = 0xb0;
constexpr usize k_reg_ldr = 0xd0;
constexpr usize k_reg_dfr = 0xe0;
constexpr usize k_reg_siv = 0xf0;

constexpr usize k_reg_timer = 0x320;
constexpr usize k_reg_timer_count = 0x380;
constexpr usize k_reg_timer_count_current = 0x390;
constexpr usize k_reg_timer_config = 0x3e0;

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
    write(k_reg_dfr, 0xffffffffu);
    write(k_reg_ldr, 0x1000000u);
    write(k_reg_tpr, 0u);
    write(k_reg_siv, 255u | 0x100u);
}

void LocalApic::send_ack() {
    write(k_reg_eoi, 0u);
}

void LocalApic::set_timer(TimerMode mode, uint8 vector) {
    write(k_reg_timer_config, 3u);
    write(k_reg_timer, vector | (static_cast<uint32>(mode) << 16u));
}

void LocalApic::set_timer_count(uint32 count) {
    write(k_reg_timer_count, count);
}

uint32 LocalApic::read_timer_count() const {
    return read<uint32>(k_reg_timer_count_current);
}
