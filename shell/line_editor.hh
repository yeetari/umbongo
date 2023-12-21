#pragma once

#include <core/key_event.hh> // IWYU pragma: keep
#include <ustd/string.hh>
#include <ustd/string_view.hh>
#include <ustd/types.hh>
#include <ustd/vector.hh>

class LineEditor {
    const ustd::StringView m_prompt;
    ustd::Vector<char> m_buffer;
    ustd::Vector<ustd::String> m_history;
    uint32_t m_cursor_pos{0};
    uint32_t m_history_pos{0};

    void clear();
    void clear_line();
    void goto_start();
    void goto_end();

public:
    explicit LineEditor(ustd::StringView prompt) : m_prompt(prompt) {}

    void begin_line();
    ustd::StringView handle_key_event(core::KeyEvent event);
};
