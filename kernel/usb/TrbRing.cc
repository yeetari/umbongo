#include <kernel/usb/TrbRing.hh>

#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/Memory.hh>
#include <ustd/Span.hh>
#include <ustd/Types.hh>

namespace usb {

TrbRing::TrbRing(bool insert_link) {
    ASSERT(reinterpret_cast<uintptr>(this) == reinterpret_cast<uintptr>(m_queue.data()));
    if (insert_link) {
        m_queue[255].data_ptr = this;
        m_queue[255].ent = true;
        m_queue[255].type = TrbType::Link;
    }
}

ustd::Span<TransferRequestBlock> TrbRing::dequeue() {
    const auto start_index = m_index;
    usize length = 0;
    for (; m_queue[m_index].cycle == m_cycle_state; length++) { // NOLINT
        m_index++;
        if (m_index >= 256) {
            m_cycle_state = !m_cycle_state;
            m_index = 0;
        }
    }
    return {m_queue.data() + start_index, length};
}

TransferRequestBlock *TrbRing::enqueue(TransferRequestBlock *in_trb) {
    in_trb->cycle = m_cycle_state;
    auto *trb = memcpy(&m_queue[m_index++], in_trb, sizeof(TransferRequestBlock));
    auto &next_trb = m_queue[m_index];
    if (next_trb.type == TrbType::Link) {
        ASSERT_PEDANTIC(next_trb.data_ptr == m_queue.data());
        next_trb.cycle = m_cycle_state;
        m_cycle_state = !m_cycle_state;
        m_index = 0;
    }
    return static_cast<TransferRequestBlock *>(trb);
}

} // namespace usb
