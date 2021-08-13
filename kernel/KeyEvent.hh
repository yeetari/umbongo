#pragma once

#include <ustd/Types.hh>

class KeyEvent {
    uint8 m_code{0};
    bool m_alt_pressed{false};
    bool m_ctrl_pressed{false};

public:
    KeyEvent() = default;
    KeyEvent(uint8 code) : m_code(code) {}
    KeyEvent(uint8 code, bool alt_pressed, bool ctrl_pressed)
        : m_code(code), m_alt_pressed(alt_pressed), m_ctrl_pressed(ctrl_pressed) {}

    uint8 code() const { return m_code; }
    bool alt_pressed() const { return m_alt_pressed; }
    bool ctrl_pressed() const { return m_ctrl_pressed; }
};
