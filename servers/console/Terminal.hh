#pragma once

#include <ustd/Types.hh>
#include <ustd/Vector.hh>

class Framebuffer;

class Line {
    Vector<char> m_chars;
    bool m_dirty{false};

public:
    explicit Line(uint32 column_count) : m_chars(column_count) {}

    void set_dirty(bool dirty) { m_dirty = dirty; }

    Vector<char> &chars() { return m_chars; }
    bool dirty() const { return m_dirty; }
};

class Terminal {
    const Framebuffer &m_fb;
    Vector<Line> m_lines;
    uint32 m_column_count;
    uint32 m_row_count;

    uint32 m_cursor_x{0};
    uint32 m_cursor_y{0};

    void set_char(uint32 row, uint32 col, char ch);

public:
    explicit Terminal(const Framebuffer &fb);

    void backspace();
    void newline();
    void move_left();
    void move_right();
    void put_char(char ch);

    void clear_cursor();
    void render();
};
