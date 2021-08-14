#include "LineEditor.hh"

#include <kernel/KeyEvent.hh>
#include <ustd/Log.hh>
#include <ustd/Memory.hh>
#include <ustd/Optional.hh>
#include <ustd/String.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>
#include <ustd/Vector.hh>

void LineEditor::clear_line() {
    for (uint32 i = 0; i < m_cursor_pos; i++) {
        m_buffer.remove(m_cursor_pos - i - 1);
        log_put_char('\b');
    }
    m_cursor_pos = 0;
}

Optional<String> LineEditor::handle_key_event(KeyEvent event) {
    if (event.character() == '\b') {
        if (m_cursor_pos == 0) {
            return {};
        }
        m_buffer.remove(--m_cursor_pos);
        log_put_char('\b');
        return {};
    }
    if (event.character() == '\n') {
        String line(m_buffer.data(), m_buffer.size());
        log_put_char('\n');
        if (line.empty()) {
            return ustd::move(line);
        }
        m_buffer.clear();
        m_cursor_pos = 0;
        m_history.push(line);
        m_history_pos = m_history.size();
        return ustd::move(line);
    }
    if (event.ctrl_pressed()) {
        if (event.character() == 'u') {
            clear_line();
        }
        return {};
    }
    if (event.code() == 0x51 || event.code() == 0x52) {
        if (event.code() == 0x51) {
            if (m_history_pos < m_history.size()) {
                m_history_pos++;
            }
        } else if (event.code() == 0x52) {
            if (m_history_pos > 0) {
                m_history_pos--;
            }
        }
        clear_line();
        if (m_history_pos < m_history.size()) {
            for (auto ch : m_history[m_history_pos]) {
                m_buffer.push(ch);
                m_cursor_pos++;
                log_put_char(ch);
            }
        }
    }
    if (event.code() == 0x50) {
        if (m_cursor_pos > 0) {
            m_cursor_pos--;
            log("\x1b[D");
        }
    }
    if (event.code() == 0x4f) {
        if (m_cursor_pos < m_buffer.size()) {
            m_cursor_pos++;
            log("\x1b[C");
        }
    }
    if (event.character() == '\0') {
        return {};
    }
    m_buffer.grow(m_buffer.size() + 1);
    for (uint32 i = m_buffer.size() - m_cursor_pos - 1; i > 0; i--) {
        m_buffer[m_cursor_pos + i] = m_buffer[m_cursor_pos + i - 1];
    }
    m_buffer[m_cursor_pos++] = event.character();
    log_put_char(event.character());
    return {};
}
