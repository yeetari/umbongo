#include <kernel/intr/IoApic.hh>

#include <kernel/intr/InterruptType.hh>
#include <ustd/Types.hh>

namespace kernel {
namespace {

constexpr usize k_reg_id = 0x0;
constexpr usize k_reg_version = 0x1;
constexpr usize k_reg_redtbl = 0x10;

} // namespace

template <>
uint32 IoApic::read(uint32 reg) {
    *m_base = reg;
    return *(m_base + 4);
}

template <>
void IoApic::write(uint32 reg, uint32 val) {
    *m_base = reg;
    *(m_base + 4) = val;
}

template <>
void IoApic::write(uint32 reg, uint64 val) {
    write<uint32>(reg, val & 0xffffffffu);
    write<uint32>(reg + 1, (val >> 32ul) & 0xffffffffu);
}

IoApic::IoApic(volatile uint32 *base, uint32 gsi_base)
    : m_base(base), m_gsi_base(gsi_base), m_id((read<uint32>(k_reg_id) >> 24u) & 0xfu),
      m_redirection_entry_count((read<uint32>(k_reg_version) >> 16u) + 1) {}

void IoApic::set_redirection_entry(uint32 gsi, uint8 vector, InterruptPolarity polarity,
                                   InterruptTriggerMode trigger_mode) {
    write<uint64>(k_reg_redtbl + gsi * 2,
                  (static_cast<uint64>(trigger_mode) << 15u) | (static_cast<uint64>(polarity) << 13u) | vector);
}

} // namespace kernel
