#pragma once

#include <ustd/types.hh>
#include <ustd/vector.hh>

class Terminal;

class EscapeParser {
    Terminal &m_default_terminal;
    Terminal &m_alternate_terminal;
    uint32_t m_current_param{0};
    bool m_in_escape{false};
    ustd::Vector<uint32_t> m_params;

public:
    EscapeParser(Terminal &default_terminal, Terminal &alternate_terminal)
        : m_default_terminal(default_terminal), m_alternate_terminal(alternate_terminal) {}

    bool parse(char ch, Terminal *&terminal);
};
