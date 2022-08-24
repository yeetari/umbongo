#include <kernel/intr/IoApic.hh>

#include <kernel/intr/InterruptType.hh>
#include <ustd/Types.hh>

namespace kernel {
namespace {

constexpr size_t k_reg_id = 0x0;
constexpr size_t k_reg_version = 0x1;
constexpr size_t k_reg_redtbl = 0x10;

} // namespace

template <>
uint32_t IoApic::read(uint32_t reg) {
    *m_base = reg;
    return *(m_base + 4);
}

template <>
void IoApic::write(uint32_t reg, uint32_t val) {
    *m_base = reg;
    *(m_base + 4) = val;
}

template <>
void IoApic::write(uint32_t reg, uint64_t val) {
    write<uint32_t>(reg, val & 0xffffffffu);
    write<uint32_t>(reg + 1, (val >> 32ul) & 0xffffffffu);
}

IoApic::IoApic(volatile uint32_t *base, uint32_t gsi_base)
    : m_base(base), m_gsi_base(gsi_base), m_id((read<uint32_t>(k_reg_id) >> 24u) & 0xfu),
      m_redirection_entry_count((read<uint32_t>(k_reg_version) >> 16u) + 1) {}

void IoApic::set_redirection_entry(uint32_t gsi, uint8_t vector, InterruptPolarity polarity,
                                   InterruptTriggerMode trigger_mode) {
    write<uint64_t>(k_reg_redtbl + gsi * 2,
                    (static_cast<uint64_t>(trigger_mode) << 15u) | (static_cast<uint64_t>(polarity) << 13u) | vector);
}

} // namespace kernel
