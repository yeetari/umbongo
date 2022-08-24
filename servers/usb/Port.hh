#pragma once

#include <ustd/Types.hh>

class Port {
    uint32_t *m_portsc;
    uint8_t m_id;

public:
    Port(uint32_t *portsc, uint8_t id) : m_portsc(portsc), m_id(id) {}

    bool connected() const;
    bool reset() const;
    uint8_t speed() const;
    uint8_t id() const { return m_id; }
};
