#pragma once

#include <kernel/KeyEvent.hh> // IWYU pragma: keep
#include <ustd/Optional.hh>
#include <ustd/String.hh>
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

class LineEditor {
    Vector<char> m_buffer;
    Vector<String> m_history;
    uint32 m_cursor_pos{0};
    uint32 m_history_pos{0};

    void clear_line();
    void goto_start();
    void goto_end();

public:
    Optional<String> handle_key_event(KeyEvent event);
};
