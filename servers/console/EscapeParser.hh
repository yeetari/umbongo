#pragma once

#include <ustd/Types.hh>
#include <ustd/Vector.hh>

class Terminal;

class EscapeParser {
    Terminal &m_default_terminal;
    Terminal &m_alternate_terminal;
    uint32 m_current_param{0};
    bool m_in_escape{false};
    Vector<uint32> m_params;

public:
    EscapeParser(Terminal &default_terminal, Terminal &alternate_terminal)
        : m_default_terminal(default_terminal), m_alternate_terminal(alternate_terminal) {}

    bool parse(char ch, Terminal *&terminal);
};
