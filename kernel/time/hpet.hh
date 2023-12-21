#pragma once

#include <ustd/types.hh>

namespace kernel {

class Hpet {
    const uint64_t m_address;
    uint64_t m_period;
    uint64_t m_previous_count{0};
    uint32_t m_32_bit_wraps{0};
    bool m_64_bit{false};

    uint64_t read_counter() const;

public:
    explicit Hpet(uint64_t address);

    void enable() const;
    void spin(uint64_t millis) const;
    uint64_t update_time();
};

} // namespace kernel
