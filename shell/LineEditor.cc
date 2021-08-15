#include "LineEditor.hh"

#include <kernel/KeyEvent.hh>
#include <ustd/Log.hh>
#include <ustd/Memory.hh>
#include <ustd/Optional.hh>
#include <ustd/String.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>
#include <ustd/Vector.hh>

void LineEditor::clear() {
    log("\x1b[J");
    m_buffer.clear();
    m_cursor_pos = 0;
    begin_line();
}

void LineEditor::clear_line() {
    for (uint32 i = 0; i < m_cursor_pos; i++) {
        m_buffer.remove(m_cursor_pos - i - 1);
        log_put_char('\b');
    }
    m_cursor_pos = 0;
}

void LineEditor::goto_start() {
    for (uint32 i = 0; i < m_cursor_pos; i++) {
        log("\x1b[D");
    }
    m_cursor_pos = 0;
}

void LineEditor::goto_end() {
    for (uint32 i = m_cursor_pos; i < m_buffer.size(); i++) {
        log("\x1b[C");
    }
    m_cursor_pos = m_buffer.size();
}

void LineEditor::begin_line() {
    log("{}", m_prompt);
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
        switch (event.character()) {
        case 'a':
            goto_start();
            break;
        case 'e':
            goto_end();
            break;
        case 'l':
            clear();
            break;
        case 'u':
            clear_line();
            break;
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
        return {};
    }
    switch (event.code()) {
    case 0x4a:
        goto_start();
        return {};
    case 0x4d:
        goto_end();
        return {};
    case 0x4f:
        if (m_cursor_pos < m_buffer.size()) {
            m_cursor_pos++;
            log("\x1b[C");
        }
        return {};
    case 0x50:
        if (m_cursor_pos > 0) {
            m_cursor_pos--;
            log("\x1b[D");
        }
        return {};
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
