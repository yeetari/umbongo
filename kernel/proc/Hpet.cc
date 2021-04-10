#include <kernel/proc/Hpet.hh>

#include <ustd/Types.hh>

namespace {

struct [[gnu::packed]] GeneralCaps {
    uint8 rev_id;
    uint8 timer_count : 5;
    bool qword : 1;
    usize : 1;
    bool legacy_replacement_capable : 1;
    uint16 vendor_id;
    uint32 period;
};

} // namespace

Hpet::Hpet(uint64 address) : m_address(address) {
    auto *caps = reinterpret_cast<volatile GeneralCaps *>(address);
    m_period = caps->period;
}

void Hpet::enable() const {
    *reinterpret_cast<volatile uint64 *>(m_address + 0x10) =
        *reinterpret_cast<volatile uint64 *>(m_address + 0x10) | 1u;
}

void Hpet::spin(uint64 millis) const {
    constexpr uint64 millis_per_femto = 1000000000000;
    uint64 trigger = read_counter() + millis * millis_per_femto / m_period;
    while (read_counter() < trigger) {
        asm volatile("pause");
    }
}

uint64 Hpet::read_counter() const {
    return *reinterpret_cast<volatile uint64 *>(m_address + 0xf0);
}
