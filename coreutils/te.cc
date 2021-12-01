#include <console/Console.hh>
#include <core/Error.hh>
#include <kernel/KeyEvent.hh>
#include <kernel/Syscall.hh>
#include <kernel/SyscallTypes.hh>
#include <ustd/Assert.hh>
#include <ustd/Log.hh>
#include <ustd/Numeric.hh>
#include <ustd/Optional.hh>
#include <ustd/String.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>
#include <ustd/Vector.hh>

namespace {

class Line {
    ustd::Vector<char> m_chars;

public:
    void put_char(char ch) { m_chars.push(ch); }
    void put_char(char ch, uint32 index) {
        // TODO: Use Vector::insert(index, elem) when available.
        m_chars.emplace_at(index, ch);
    }
    void del_char(uint32 index) { m_chars.remove(index); }

    char character(uint32 index) const { return m_chars[index]; }
    uint32 column_count() const { return m_chars.size(); }
    const char *data() const { return m_chars.data(); }
};

class Editor {
    const uint32 m_screen_column_count;
    const uint32 m_screen_row_count;
    ustd::String m_path;
    ustd::Vector<Line> m_lines;
    uint32 m_cursor_x{0};
    uint32 m_cursor_y{0};
    uint32 m_file_x{0};
    uint32 m_file_y{0};
    ustd::Optional<uint32> m_saved_cursor_x;

public:
    Editor(uint32 screen_column_count, uint32 screen_row_count);
    Editor(const Editor &) = delete;
    Editor(Editor &&) = delete;
    ~Editor();

    Editor &operator=(const Editor &) = delete;
    Editor &operator=(Editor &&) = delete;

    bool load(ustd::String &&path);
    bool read_key();
    void render();
};

Editor::Editor(uint32 screen_column_count, uint32 screen_row_count)
    : m_screen_column_count(screen_column_count), m_screen_row_count(screen_row_count) {
    ustd::printf("\x1b[?1049h");
}

Editor::~Editor() {
    ustd::printf("\x1b[?1049l");
}

bool Editor::load(ustd::String &&path) {
    // TODO: Use core::File.
    m_path = ustd::move(path);
    auto fd = Syscall::invoke(Syscall::open, m_path.data(), OpenMode::Create);
    if (fd < 0) {
        printf("te: {}: {}\n", m_path.view(), core::error_string(fd));
        return false;
    }

    auto *line = &m_lines.emplace();
    auto size = Syscall::invoke<usize>(Syscall::size, fd);
    for (usize i = 0; i < size; i++) {
        char ch = 0;
        Syscall::invoke(Syscall::read, fd, &ch, 1);
        if (ch == '\n') {
            line = &m_lines.emplace();
            continue;
        }
        line->put_char(ch);
    }
    if (m_lines.size() > 1) {
        m_lines.pop();
    }
    Syscall::invoke(Syscall::close, fd);
    return true;
}

bool Editor::read_key() {
    KeyEvent event;
    ssize rc = Syscall::invoke(Syscall::read, 0, &event, sizeof(KeyEvent));
    if (rc != sizeof(KeyEvent)) {
        printf("{}\n", core::error_string(rc));
        return false;
    }
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
        // TODO: Use core::File.
        auto fd = Syscall::invoke(Syscall::open, m_path.data(), OpenMode::Truncate);
        ENSURE(fd >= 0);
        for (const auto &line : m_lines) {
            const char newline = '\n';
            Syscall::invoke(Syscall::write, fd, line.data(), line.column_count());
            Syscall::invoke(Syscall::write, fd, &newline, 1);
        }
        Syscall::invoke(Syscall::close, fd);
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
            for (uint32 i = 0; i < line.column_count(); i++) {
                m_lines[m_cursor_y - 1].put_char(line.character(i));
            }
            m_lines.remove(m_cursor_y--);
        }
        return true;
    }
    if (event.character() == '\n') {
        m_lines.emplace_at(m_cursor_y++);
        if (m_cursor_x > 0) {
            for (uint32 i = 0; i < m_cursor_x; i++) {
                m_lines[m_cursor_y - 1].put_char(m_lines[m_cursor_y].character(i));
            }
            for (uint32 i = 0; i < m_cursor_x; i++) {
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

    ustd::printf("\x1b[J");
    for (uint32 row = 0; row < m_screen_row_count; row++) {
        uint32 file_row = row + m_file_y;
        if (file_row >= m_lines.size()) {
            break;
        }
        const auto &line = m_lines[file_row];
        for (uint32 col = 0; col < m_screen_column_count - 1; col++) {
            uint32 file_col = col + m_file_x;
            if (file_col >= line.column_count()) {
                break;
            }
            put_char(line.character(file_col));
        }
        if (row != m_screen_row_count - 1) {
            put_char('\n');
        }
    }
    ustd::printf("\x1b[{};{}H", m_cursor_y - m_file_y, m_cursor_x - m_file_x);
}

} // namespace

usize main(usize argc, const char **argv) {
    if (argc != 2) {
        ustd::printf("Usage: {} <file>\n", argv[0]);
        return 0;
    }
    auto terminal_size = console::terminal_size();
    Editor editor(terminal_size.column_count, terminal_size.row_count);
    if (!editor.load(argv[1])) {
        return 1;
    }
    do {
        editor.render();
    } while (editor.read_key());
    return 0;
}
