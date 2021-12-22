#pragma once

#include <ustd/Types.hh>

namespace kernel {

class Hpet {
    const uint64 m_address;
    uint64 m_period;
    uint64 m_previous_count{0};
    uint32 m_32_bit_wraps{0};
    bool m_64_bit{false};

    uint64 read_counter() const;

public:
    explicit Hpet(uint64 address);

    void enable() const;
    void spin(uint64 millis) const;
    uint64 update_time();
};

} // namespace kernel
