#include "LineEditor.hh"

#include <core/KeyEvent.hh>
#include <core/Print.hh>
#include <core/Process.hh>
#include <ustd/String.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>
#include <ustd/Vector.hh>

void LineEditor::clear() {
    core::print("\x1b[J");
    m_buffer.clear();
    m_cursor_pos = 0;
    begin_line();
}

void LineEditor::clear_line() {
    for (uint32 i = 0; i < m_cursor_pos; i++) {
        m_buffer.remove(m_cursor_pos - i - 1);
        core::put_char('\b');
    }
    m_cursor_pos = 0;
}

void LineEditor::goto_start() {
    for (uint32 i = 0; i < m_cursor_pos; i++) {
        core::print("\x1b[D");
    }
    m_cursor_pos = 0;
}

void LineEditor::goto_end() {
    for (uint32 i = m_cursor_pos; i < m_buffer.size(); i++) {
        core::print("\x1b[C");
    }
    m_cursor_pos = m_buffer.size();
}

void LineEditor::begin_line() {
    core::print("\x1b[38;2;179;179;255m");
    auto cwd = core::cwd();
    ustd::StringView cwd_view = cwd;
    if (cwd.length() >= 5) {
        ustd::StringView start(cwd.data(), 5);
        if (start == "/home") {
            core::print("~");
            cwd_view = ustd::StringView(cwd.data() + 5, cwd.length() - 5);
        }
    }
    core::print("{} {}", cwd_view, m_prompt);
    core::print("\x1b[38;2;255;255;255m");
}

ustd::StringView LineEditor::handle_key_event(core::KeyEvent event) {
    if (event.character() == '\b') {
        if (m_cursor_pos == 0) {
            return {};
        }
        m_buffer.remove(--m_cursor_pos);
        core::put_char('\b');
        return {};
    }
    if (event.character() == '\n') {
        auto line = ustd::String::copy_raw(m_buffer.data(), m_buffer.size());
        core::put_char('\n');
        if (line.empty()) {
            return "\n";
        }
        m_buffer.clear();
        m_cursor_pos = 0;
        m_history.push(ustd::move(line));
        m_history_pos = m_history.size();
        return m_history.last();
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
        switch (event.code()) {
        case 0x4f:
            if (m_cursor_pos == m_buffer.size()) {
                break;
            }
            m_cursor_pos++;
            core::print("\x1b[C");
            for (uint32 i = m_cursor_pos; i < m_buffer.size(); i++) {
                m_cursor_pos++;
                core::print("\x1b[C");
                if (m_cursor_pos == m_buffer.size() || m_buffer[m_cursor_pos] == ' ') {
                    break;
                }
            }
            break;
        case 0x50:
            if (m_cursor_pos == 0) {
                break;
            }
            m_cursor_pos--;
            core::print("\x1b[D");
            for (uint32 i = m_cursor_pos; i > 0; i--) {
                m_cursor_pos--;
                core::print("\x1b[D");
                if (m_buffer[m_cursor_pos] == ' ') {
                    break;
                }
            }
            if (m_cursor_pos != 0) {
                m_cursor_pos++;
                core::print("\x1b[C");
            }
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
                core::put_char(ch);
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
            core::print("\x1b[C");
        }
        return {};
    case 0x50:
        if (m_cursor_pos > 0) {
            m_cursor_pos--;
            core::print("\x1b[D");
        }
        return {};
    }
    if (event.character() == '\0') {
        return {};
    }
    m_buffer.ensure_size(m_buffer.size() + 1);
    for (uint32 i = m_buffer.size() - m_cursor_pos - 1; i > 0; i--) {
        m_buffer[m_cursor_pos + i] = m_buffer[m_cursor_pos + i - 1];
    }
    m_buffer[m_cursor_pos++] = event.character();
    core::put_char(event.character());
    return {};
}
