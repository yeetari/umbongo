#include "port.hh"

#include <mmio/mmio.hh>
#include <ustd/types.hh>

bool Port::connected() const {
    return m_portsc != nullptr && (mmio::read(*m_portsc) & 1u) != 0u;
}

bool Port::reset() const {
    mmio::write(*m_portsc, (1u << 9u) | (1u << 4u));
    return mmio::wait_timeout(*m_portsc, (1u << 21u) | (1u << 4u), 1u << 21u, 500);
}

uint8_t Port::speed() const {
    return (mmio::read(*m_portsc) >> 10u) & 0xfu;
}
