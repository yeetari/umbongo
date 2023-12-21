#include <console/console.hh>
#include <core/error.hh>
#include <core/file.hh>
#include <core/key_event.hh>
#include <core/print.hh>
#include <system/syscall.hh>
#include <ustd/numeric.hh>
#include <ustd/optional.hh>
#include <ustd/result.hh>
#include <ustd/string.hh>
#include <ustd/try.hh>
#include <ustd/types.hh>
#include <ustd/utility.hh>
#include <ustd/vector.hh>

namespace {

class Line {
    ustd::Vector<char> m_chars;

public:
    void put_char(char ch) { m_chars.push(ch); }
    void put_char(char ch, uint32_t index) {
        // TODO: Use Vector::insert(index, elem) when available.
        m_chars.emplace_at(index, ch);
    }
    void del_char(uint32_t index) { m_chars.remove(index); }

    char character(uint32_t index) const { return m_chars[index]; }
    uint32_t column_count() const { return m_chars.size(); }
    const char *data() const { return m_chars.data(); }
};

class Editor {
    const uint32_t m_screen_column_count;
    const uint32_t m_screen_row_count;
    ustd::String m_path;
    ustd::Vector<Line> m_lines;
    uint32_t m_cursor_x{0};
    uint32_t m_cursor_y{0};
    uint32_t m_file_x{0};
    uint32_t m_file_y{0};
    ustd::Optional<uint32_t> m_saved_cursor_x;

public:
    Editor(uint32_t screen_column_count, uint32_t screen_row_count);
    Editor(const Editor &) = delete;
    Editor(Editor &&) = delete;
    ~Editor();

    Editor &operator=(const Editor &) = delete;
    Editor &operator=(Editor &&) = delete;

    ustd::Result<void, ub_error_t> load(ustd::String &&path);
    bool read_key();
    void render();
};

Editor::Editor(uint32_t screen_column_count, uint32_t screen_row_count)
    : m_screen_column_count(screen_column_count), m_screen_row_count(screen_row_count) {
    core::print("\x1b[?1049h");
}

Editor::~Editor() {
    core::print("\x1b[?1049l");
}

ustd::Result<void, ub_error_t> Editor::load(ustd::String &&path) {
    m_path = ustd::move(path);
    auto file = TRY(core::File::open(m_path, UB_OPEN_MODE_CREATE));
    auto size = TRY(file.size());
    auto *line = &m_lines.emplace();
    for (size_t i = 0; i < size; i++) {
        auto ch = TRY(file.read<char>());
        if (ch == '\n') {
            line = &m_lines.emplace();
            continue;
        }
        line->put_char(ch);
    }
    if (m_lines.size() > 1) {
        m_lines.pop();
    }
    return {};
}

bool Editor::read_key() {
    // TODO: Global stdin file to .read<KeyEvent>() from.
    core::KeyEvent event;
    EXPECT(system::syscall<ssize_t>(UB_SYS_read, 0, &event, sizeof(core::KeyEvent)));
    if (event.code() >= 0x4f && event.code() <= 0x52) {
        if (event.ctrl_pressed()) {
            return true;
        }
        if (event.alt_pressed()) {
            if (event.code() == 0x51 && m_cursor_y < m_lines.size() - 1) {
                ustd::swap(m_lines[m_cursor_y], m_lines[m_cursor_y + 1]);
                m_cursor_y++;
            } else if (event.code() == 0x52 && m_cursor_y > 0) {
                ustd::swap(m_lines[m_cursor_y], m_lines[m_cursor_y - 1]);
                m_cursor_y--;
            }
            return true;
        }
        switch (event.code()) {
        case 0x4f:
            if (m_cursor_x < m_lines[m_cursor_y].column_count()) {
                m_cursor_x++;
            } else if (m_cursor_y < m_lines.size()) {
                m_cursor_x = 0;
                m_cursor_y++;
            }
            break;
        case 0x50:
            if (m_cursor_x > 0) {
                m_cursor_x--;
            } else if (m_cursor_y > 0) {
                m_cursor_x = m_lines[--m_cursor_y].column_count();
            }
            break;
        case 0x51:
            if (m_cursor_y < m_lines.size()) {
                m_cursor_y++;
            }
            break;
        case 0x52:
            if (m_cursor_y > 0) {
                m_cursor_y--;
            }
            break;
        }
        if (m_cursor_y == m_lines.size()) {
            if (m_lines[m_cursor_y - 1].column_count() == 0) {
                m_cursor_y--;
            } else {
                m_lines.emplace();
            }
        }
        if (event.code() == 0x4f || event.code() == 0x50) {
            m_saved_cursor_x.clear();
        } else if ((event.code() == 0x51 || event.code() == 0x52) && m_saved_cursor_x) {
            m_cursor_x = ustd::min(*m_saved_cursor_x, m_lines[m_cursor_y].column_count());
        }
        if (m_cursor_x > m_lines[m_cursor_y].column_count()) {
            // TODO: Optional::operator=
            m_saved_cursor_x.emplace(m_cursor_x);
            m_cursor_x = m_lines[m_cursor_y].column_count();
        }
        return true;
    }
    if ((event.ctrl_pressed() && event.character() == 'a') || event.code() == 0x4a) {
        m_cursor_x = 0;
        m_saved_cursor_x.clear();
        return true;
    }
    if ((event.ctrl_pressed() && event.character() == 'e') || event.code() == 0x4d) {
        m_cursor_x = m_lines[m_cursor_y].column_count();
        m_saved_cursor_x.clear();
        return true;
    }
    if (event.ctrl_pressed() && event.character() == 'q') {
        return false;
    }
    if (event.ctrl_pressed() && event.character() == 'x') {
        auto file = EXPECT(core::File::open(m_path, UB_OPEN_MODE_TRUNCATE));
        for (const auto &line : m_lines) {
            EXPECT(file.write({line.data(), line.column_count()}));
            EXPECT(file.write('\n'));
        }
        return false;
    }
    if (event.ctrl_pressed()) {
        return true;
    }
    if (event.character() == '\b') {
        auto &line = m_lines[m_cursor_y];
        if (m_cursor_x > 0) {
            line.del_char(--m_cursor_x);
        } else if (m_cursor_y > 0) {
            m_cursor_x = m_lines[m_cursor_y - 1].column_count();
            for (uint32_t i = 0; i < line.column_count(); i++) {
                m_lines[m_cursor_y - 1].put_char(line.character(i));
            }
            m_lines.remove(m_cursor_y--);
        }
        return true;
    }
    if (event.character() == '\n') {
        m_lines.emplace_at(m_cursor_y++);
        if (m_cursor_x > 0) {
            for (uint32_t i = 0; i < m_cursor_x; i++) {
                m_lines[m_cursor_y - 1].put_char(m_lines[m_cursor_y].character(i));
            }
            for (uint32_t i = 0; i < m_cursor_x; i++) {
                m_lines[m_cursor_y].del_char(0);
            }
        }
        m_cursor_x = 0;
        return true;
    }
    if (event.character() != '\0') {
        auto &line = m_lines[m_cursor_y];
        line.put_char(event.character(), m_cursor_x++);
    }
    return true;
}

void Editor::render() {
    if (m_cursor_x < m_file_x) {
        m_file_x = m_cursor_x;
    } else if (m_cursor_x >= m_file_x + (m_screen_column_count - 1)) {
        m_file_x = m_cursor_x - m_screen_column_count + 2;
    }
    if (m_cursor_y < m_file_y) {
        m_file_y = m_cursor_y;
    } else if (m_cursor_y >= m_file_y + (m_screen_row_count - 1)) {
        m_file_y = m_cursor_y - (m_screen_row_count - 1);
    }

    core::print("\x1b[J");
    for (uint32_t row = 0; row < m_screen_row_count; row++) {
        uint32_t file_row = row + m_file_y;
        if (file_row >= m_lines.size()) {
            break;
        }
        const auto &line = m_lines[file_row];
        for (uint32_t col = 0; col < m_screen_column_count - 1; col++) {
            uint32_t file_col = col + m_file_x;
            if (file_col >= line.column_count()) {
                break;
            }
            core::put_char(line.character(file_col));
        }
        if (row != m_screen_row_count - 1) {
            core::put_char('\n');
        }
    }
    core::print("\x1b[{};{}H", m_cursor_y - m_file_y, m_cursor_x - m_file_x);
}

} // namespace

size_t main(size_t argc, const char **argv) {
    if (argc != 2) {
        core::println("Usage: {} <file>", argv[0]);
        return 0;
    }
    auto terminal_size = console::terminal_size();
    Editor editor(terminal_size.column_count, terminal_size.row_count);
    if (auto result = editor.load(argv[1]); result.is_error()) {
        core::print("\x1b[?1049l");
        core::println("te: {}: {}", argv[1], core::error_string(result.error()));
        return 1;
    }
    do {
        editor.render();
    } while (editor.read_key());
    return 0;
}
