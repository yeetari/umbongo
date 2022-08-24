#include "TrbRing.hh"

#include <core/Error.hh>
#include <mmio/Mmio.hh>
#include <ustd/Assert.hh>
#include <ustd/Result.hh>
#include <ustd/Span.hh>
#include <ustd/Try.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>

namespace {

constexpr size_t k_ring_length = 256;

} // namespace

ustd::Result<ustd::UniquePtr<TrbRing>, core::SysError> TrbRing::create(bool insert_link) {
    auto *ring_dma = TRY(mmio::alloc_dma_array<RawTrb>(k_ring_length));
    return ustd::make_unique<TrbRing>(ring_dma, insert_link);
}

TrbRing::TrbRing(RawTrb *ring, bool insert_link) : m_ring(ring) {
    if (insert_link) {
        m_ring[k_ring_length - 1] = {
            .data = EXPECT(physical_base()),
            // evaluate_next = toggle cycle bit for link TRBs.
            .evaluate_next = true,
            .type = TrbType::Link,
        };
    }
}

ustd::Span<RawTrb> TrbRing::dequeue() {
    const size_t start_index = m_index;
    size_t length = 0;
    for (; static_cast<bool>(m_ring[m_index].cycle) == m_cycle_state; length++) {
        m_index++;
        if (m_index >= k_ring_length) {
            m_cycle_state = !m_cycle_state;
            m_index = 0;
        }
    }
    return {m_ring + start_index, length};
}

RawTrb &TrbRing::enqueue(const RawTrb &trb) {
    auto &enqueued = (m_ring[m_index++] = trb);
    enqueued.cycle = m_cycle_state;
    if (auto &link_trb = m_ring[m_index]; link_trb.type == TrbType::Link) {
        // TODO: Support chaining across segment boundary.
        ENSURE(!trb.chain);
        link_trb.cycle = m_cycle_state;
        m_cycle_state = !m_cycle_state;
        m_index = 0;
    }
    asm volatile("sfence" ::: "memory");
    return enqueued;
}

ustd::Result<uintptr_t, core::SysError> TrbRing::physical_base() const {
    return TRY(mmio::virt_to_phys(m_ring));
}

ustd::Result<uintptr_t, core::SysError> TrbRing::physical_head() const {
    return TRY(mmio::virt_to_phys(&m_ring[m_index]));
}

RawTrb &TrbRing::operator[](size_t index) const {
    ASSERT(index < k_ring_length);
    return m_ring[index];
}
