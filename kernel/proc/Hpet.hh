#pragma once

#include <ustd/Types.hh>

class Hpet {
    const uint64 m_address;
    uint64 m_period;

public:
    explicit Hpet(uint64 address);

    void enable() const;
    void spin(uint64 millis) const;
    uint64 read_counter() const;
};
