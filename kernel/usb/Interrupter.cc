#include <kernel/usb/Interrupter.hh>

#include <kernel/usb/TrbRing.hh>
#include <ustd/Memory.hh>
#include <ustd/Types.hh>

namespace usb {
namespace {

constexpr uint16 k_reg_iman = 0x00;
constexpr uint16 k_reg_imod = 0x04;
constexpr uint16 k_reg_size = 0x08;
constexpr uint16 k_reg_base = 0x10;
constexpr uint16 k_reg_dptr = 0x18;

} // namespace

template <typename T>
T Interrupter::read_reg(uint16 offset) const {
    return *reinterpret_cast<volatile T *>(m_regs + offset);
}

template <typename T>
void Interrupter::write_reg(uint16 offset, T value) {
    *reinterpret_cast<volatile T *>(m_regs + offset) = value;
}

Interrupter::Interrupter(uintptr regs, TrbRing *event_ring) : m_regs(regs) {
    m_segment_table = new (ustd::align_val_t(64_KiB)) EventRingSegment[1];
    m_segment_table[0].base = event_ring;
    m_segment_table[0].trb_count = 256;

    // Enable interrupts (bit 1) and clear any pending interrupts (bit 0).
    write_reg(k_reg_iman, 0b11u);

    // Limit number of interrupts to 4000 per second.
    write_reg(k_reg_imod, 1024);

    // Set the size of the segment table, the current dequeue pointer and the address of the segment table. Note that
    // the base register should always be written last since it triggers interrupter initialisation.
    write_reg(k_reg_size, 1);
    write_reg(k_reg_dptr, reinterpret_cast<uint64>(event_ring) | (1u << 3u));
    write_reg(k_reg_base, m_segment_table);
}

void Interrupter::acknowledge(TransferRequestBlock *trb) {
    write_reg(k_reg_dptr, reinterpret_cast<uint64>(trb) | (1u << 3u));
}

void Interrupter::clear_pending() {
    write_reg(k_reg_iman, 0b11u);
}

bool Interrupter::interrupt_pending() {
    return (read_reg<uint32>(k_reg_iman) & 1u) != 0;
}

} // namespace usb
