#include <kernel/time/Hpet.hh>

#include <ustd/Types.hh>

namespace kernel {
namespace {

struct [[gnu::packed]] GeneralCaps {
    uint32 attributes;
    uint32 period;
};

} // namespace

Hpet::Hpet(uint64 address) : m_address(address) {
    const auto *caps = reinterpret_cast<const volatile GeneralCaps *>(address);
    m_period = caps->period;
    m_64_bit = (caps->attributes & (1u << 13u)) != 0u;
}

void Hpet::enable() const {
    *reinterpret_cast<volatile uint64 *>(m_address + 0x10) =
        *reinterpret_cast<volatile uint64 *>(m_address + 0x10) | 1u;
}

uint64 Hpet::read_counter() const {
    auto value = *reinterpret_cast<volatile uint64 *>(m_address + 0xf0);
    if (m_64_bit) {
        value |= (static_cast<uint64>(m_32_bit_wraps) << 32u);
    }
    return value;
}

void Hpet::spin(uint64 millis) const {
    constexpr uint64 millis_per_femto = 1000000000000;
    uint64 trigger = read_counter() + millis * millis_per_femto / m_period;
    while (read_counter() < trigger) {
        asm volatile("pause");
    }
}

uint64 Hpet::update_time() {
    uint64 current_count = read_counter();
    uint64 delta_count = 0;
    if (current_count >= m_previous_count) {
        delta_count = current_count - m_previous_count;
    } else {
        delta_count = m_previous_count - current_count;
        if (!m_64_bit) {
            m_32_bit_wraps++;
        }
    }
    m_previous_count = current_count;
    return (delta_count * m_period) / 1000000ul;
}

} // namespace kernel
