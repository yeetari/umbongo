#include <kernel/usb/Port.hh>

#include <kernel/Port.hh>
#include <kernel/usb/HostController.hh>
#include <ustd/Assert.hh>
#include <ustd/Types.hh>

namespace usb {

bool Port::initialise() {
    m_status = reinterpret_cast<volatile uint32 *>(m_controller->op_offset() + 0x400 + m_number * 16);

    // Enable power if it's not already.
    if ((*m_status & (1u << 9u)) == 0) {
        *m_status = 1u << 9u;
        for (usize i = 0; i < 250; i++) {
            port_write(0x80, 0);
        }
    }

    // Port is not powering.
    if ((*m_status & (1u << 9u)) == 0) {
        return false;
    }

    // Clear status bits.
    *m_status = (1u << 22u) | (1u << 21u) | (1u << 20u) | (1u << 18u) | (1u << 17u) | (1u << 9u);
    return true;
}

bool Port::reset() {
    usize reset_timeout = 500;
    *m_status = (1u << 9u) | (1u << 4u);
    while ((*m_status & (1u << 4u)) != 0 || (*m_status & (1u << 21u)) == 0) {
        if (reset_timeout-- == 0) {
            return false;
        }
        port_write(0x80, 0);
    }
    return true;
}

bool Port::connected() const {
    return (*m_status & 1u) != 0;
}

bool Port::enabled() const {
    return (*m_status & (1u << 1u)) != 0;
}

bool Port::initialised() const {
    return m_status != nullptr;
}

uint8 Port::speed() const {
    return (*m_status >> 10u) & 0xfu;
}

} // namespace usb
