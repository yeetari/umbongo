#pragma once

#include <kernel/KeyEvent.hh> // IWYU pragma: keep
#include <ustd/String.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

class LineEditor {
    const ustd::StringView m_prompt;
    ustd::Vector<char> m_buffer;
    ustd::Vector<ustd::String> m_history;
    uint32 m_cursor_pos{0};
    uint32 m_history_pos{0};

    void clear();
    void clear_line();
    void goto_start();
    void goto_end();

public:
    explicit LineEditor(ustd::StringView prompt) : m_prompt(prompt) {}

    void begin_line();
    ustd::StringView handle_key_event(KeyEvent event);
};
