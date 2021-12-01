#include "EscapeParser.hh"

#include "Terminal.hh"

#include <ustd/Log.hh>
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

bool EscapeParser::parse(char ch, Terminal *&terminal) {
    if (ch == '\x1b') {
        m_in_escape = true;
        m_params.clear();
        return true;
    }
    if (!m_in_escape) {
        return false;
    }
    if (ch == '[' || ch == '?') {
        return true;
    }
    if (ch - '0' < 10) {
        m_current_param *= 10;
        m_current_param += (static_cast<unsigned char>(ch) - '0');
        return true;
    }
    if (ch == ';') {
        m_params.push(m_current_param);
        m_current_param = 0;
        return true;
    }
    m_params.push(m_current_param);
    m_current_param = 0;
    m_in_escape = false;
    switch (ch) {
    case 'D':
        terminal->move_left();
        break;
    case 'C':
        terminal->move_right();
        break;
    case 'H':
        if (m_params.size() != 2) {
            ustd::dbgln("CUP doesn't have row and column");
            break;
        }
        terminal->set_cursor(m_params[1], m_params[0]);
        break;
    case 'J':
        terminal->clear();
        break;
    case 'h':
        if (m_params.size() != 1) {
            ustd::dbgln("Private mode enable doesn't have code");
            break;
        }
        if (m_params[0] != 1049) {
            ustd::dbgln("Unknown private code {}", m_params[0]);
            break;
        }
        terminal = &m_alternate_terminal;
        terminal->clear();
        break;
    case 'l':
        if (m_params.size() != 1) {
            ustd::dbgln("Private mode disable doesn't have code");
            break;
        }
        if (m_params[0] != 1049) {
            ustd::dbgln("Unknown private code {}", m_params[0]);
            break;
        }
        terminal = &m_default_terminal;
        terminal->set_dirty();
        break;
    case 'm':
        if (m_params.empty()) {
            m_params.push(0);
        }
        switch (m_params[0]) {
        case 38: // Set foreground colour
            if (m_params.size() < 2) {
                ustd::dbgln("SGR has no colour type");
                break;
            }
            if (m_params[1] != 2) {
                ustd::dbgln("Unknown colour type {}", m_params[1]);
                break;
            }
            if (m_params.size() != 5) {
                ustd::dbgln("Incorrect number of parameters for 24-bit colour set");
                break;
            }
            terminal->set_colour(m_params[2], m_params[3], m_params[4]);
            break;
        default:
            ustd::dbgln("Unknown SGR code {}", m_params[0]);
            break;
        }
        break;
    default:
        ustd::dbgln("Unknown CSI code {:c}", ch);
        break;
    }
    return true;
}
