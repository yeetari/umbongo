#pragma once

#include <ustd/Types.hh>

namespace pci {

class Bus {
    const uintptr m_segment_base;
    const uint16 m_segment_num;
    const uint8 m_num;

public:
    Bus(uintptr segment_base, uint16 segment_num, uint8 num)
        : m_segment_base(segment_base), m_segment_num(segment_num), m_num(num) {}

    uintptr segment_base() const { return m_segment_base; }
    uint16 segment_num() const { return m_segment_num; }
    uint8 num() const { return m_num; }
};

} // namespace pci
