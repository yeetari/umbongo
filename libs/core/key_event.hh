#pragma once

#include <ustd/types.hh>

namespace core {

class KeyEvent {
    uint8_t m_code{0};
    char m_character{0};
    bool m_alt_pressed{false};
    bool m_ctrl_pressed{false};

public:
    KeyEvent() = default;
    KeyEvent(uint8_t code, char character, bool alt_pressed, bool ctrl_pressed)
        : m_code(code), m_character(character), m_alt_pressed(alt_pressed), m_ctrl_pressed(ctrl_pressed) {}

    uint8_t code() const { return m_code; }
    char character() const { return m_character; }
    bool alt_pressed() const { return m_alt_pressed; }
    bool ctrl_pressed() const { return m_ctrl_pressed; }
};

} // namespace core
