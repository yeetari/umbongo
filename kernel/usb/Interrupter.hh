#pragma once

#include <ustd/Types.hh>

namespace usb {

struct TransferRequestBlock;
class TrbRing;

struct [[gnu::packed]] EventRingSegment {
    void *base;
    uint16 trb_count;
    usize : 48;
};

class Interrupter {
    const uintptr m_regs;
    EventRingSegment *m_segment_table;

    template <typename T>
    T read_reg(uint16 offset) const;

    template <typename T>
    void write_reg(uint16 offset, T value);

public:
    Interrupter(uintptr regs, TrbRing *event_ring);

    void acknowledge(TransferRequestBlock *trb);
    void clear_pending();
    bool interrupt_pending();
};

} // namespace usb
