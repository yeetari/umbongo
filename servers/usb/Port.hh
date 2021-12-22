#pragma once

#include <ustd/Types.hh>

class Port {
    uint32 *m_portsc;
    uint8 m_id;

public:
    Port(uint32 *portsc, uint8 id) : m_portsc(portsc), m_id(id) {}

    bool connected() const;
    bool reset() const;
    uint8 speed() const;
    uint8 id() const { return m_id; }
};
