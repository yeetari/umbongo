#pragma once

#include <ustd/types.hh>
#include <ustd/vector.hh>

class Framebuffer;

class Line {
    ustd::Vector<char> m_chars;
    ustd::Vector<uint32_t> m_colours;
    bool m_dirty{false};

public:
    explicit Line(uint32_t column_count) : m_chars(column_count), m_colours(column_count) {}

    void set_dirty(bool dirty) { m_dirty = dirty; }

    ustd::Vector<char> &chars() { return m_chars; }
    ustd::Vector<uint32_t> &colours() { return m_colours; }
    bool dirty() const { return m_dirty; }
};

class Terminal {
    Framebuffer &m_fb;
    ustd::Vector<Line> m_lines;
    uint32_t m_column_count;
    uint32_t m_row_count;

    uint32_t m_cursor_x{0};
    uint32_t m_cursor_y{0};
    uint32_t m_colour{0xffffffff};

    void set_char(uint32_t row, uint32_t col, char ch);

public:
    explicit Terminal(Framebuffer &fb);

    void backspace();
    void clear();
    void newline();
    void move_left();
    void move_right();
    void put_char(char ch);

    void clear_cursor();
    void render();
    void set_colour(uint32_t r, uint32_t g, uint32_t b);
    void set_cursor(uint32_t x, uint32_t y);
    void set_dirty();

    uint32_t column_count() const { return m_column_count; }
    uint32_t row_count() const { return m_row_count; }
};
