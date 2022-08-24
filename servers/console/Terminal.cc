#include "Terminal.hh"

#include "Framebuffer.hh"

#include <kernel/Font.hh>
#include <ustd/Algorithm.hh>
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

Terminal::Terminal(Framebuffer &fb) : m_fb(fb) {
    m_column_count = fb.width() / g_font.advance();
    m_row_count = fb.height() / g_font.line_height();
    m_lines.ensure_size(m_row_count, m_column_count);
}

void Terminal::set_char(uint32_t row, uint32_t col, char ch) {
    auto &line = m_lines[row];
    line.set_dirty(true);
    if (ch == '\b') {
        __builtin_memcpy(&line.chars()[col], &line.chars()[col + 1], line.chars().size() - col - 1);
        __builtin_memcpy(&line.colours()[col], &line.colours()[col + 1], line.colours().size() - col - 1);
        return;
    }
    for (uint32_t i = line.chars().size() - col - 1; i > 0; i--) {
        line.chars()[col + i] = line.chars()[col + i - 1];
        line.colours()[col + i] = line.colours()[col + i - 1];
    }
    line.chars()[col] = ch;
}

void Terminal::backspace() {
    if (m_cursor_x == 0) {
        m_cursor_x = m_column_count - 1;
        if (m_cursor_y > 0) {
            m_cursor_y--;
        }
    }
    set_char(m_cursor_y, --m_cursor_x, '\b');
}

void Terminal::clear() {
    for (auto &line : m_lines) {
        line.set_dirty(true);
        ustd::fill(line.chars(), ' ');
    }
    m_cursor_x = 0;
    m_cursor_y = 0;
}

void Terminal::newline() {
    m_cursor_x = 0;
    m_cursor_y++;
    if (m_cursor_y == m_row_count) {
        m_cursor_y--;
        m_lines.remove(0);
        m_lines.emplace(m_column_count);
        for (auto &line : m_lines) {
            line.set_dirty(true);
        }
    }
}

void Terminal::move_left() {
    m_cursor_x--;
}

void Terminal::move_right() {
    m_cursor_x++;
}

void Terminal::put_char(char ch) {
    if (m_cursor_x == m_column_count - 1) {
        newline();
    }
    set_char(m_cursor_y, m_cursor_x, ch);
    m_lines[m_cursor_y].colours()[m_cursor_x++] = m_colour;
}

void Terminal::clear_cursor() {
    m_lines[m_cursor_y].set_dirty(true);
    for (uint32_t y = 0; y < g_font.line_height() - 2; y++) {
        for (uint32_t x = 0; x < g_font.advance(); x++) {
            m_fb.set(m_cursor_x * g_font.advance() + x, m_cursor_y * g_font.line_height() + y, 0);
        }
    }
}

void Terminal::render() {
    for (uint32_t row = 0; row < m_lines.size(); row++) {
        auto &line = m_lines[row];
        if (!line.dirty()) {
            continue;
        }
        m_fb.clear_region(0, row * g_font.line_height(), m_fb.width(), g_font.line_height());
    }
    m_lines[m_cursor_y].set_dirty(true);
    for (uint32_t y = 0; y < g_font.line_height() - 2; y++) {
        for (uint32_t x = 0; x < g_font.advance(); x++) {
            m_fb.set(m_cursor_x * g_font.advance() + x, m_cursor_y * g_font.line_height() + y, 0xffffffff);
        }
    }
    for (uint32_t row = 0; row < m_lines.size(); row++) {
        auto &line = m_lines[row];
        if (!line.dirty()) {
            continue;
        }
        line.set_dirty(false);
        for (uint32_t col = 0; col < line.chars().size(); col++) {
            const auto ch = line.chars()[col];
            if (ch == '\0' || ch == ' ') {
                continue;
            }
            const auto colour = line.colours()[col];
            const auto *glyph = g_font.glyph(ch);
            for (uint32_t y1 = 0; y1 < glyph->height; y1++) {
                for (uint32_t x1 = 0; x1 < glyph->width; x1++) {
                    const uint32_t intensity = glyph->bitmap[y1 * glyph->width + x1];
                    const auto x2 =
                        static_cast<uint32_t>(static_cast<int32_t>(x1 + (col * g_font.advance())) + glyph->left);
                    const auto y2 = static_cast<uint32_t>(
                        static_cast<int32_t>(y1 + (row * g_font.line_height()) + g_font.ascender()) - glyph->top);
                    const uint32_t b = (((colour >> 0u) & 0xffu) * intensity) / 255;
                    const uint32_t g = (((colour >> 8u) & 0xffu) * intensity) / 255;
                    const uint32_t r = (((colour >> 16u) & 0xffu) * intensity) / 255;
                    m_fb.set(x2, y2, b | (g << 8u) | (r << 16u));
                }
            }
        }
    }
}

void Terminal::set_colour(uint32_t r, uint32_t g, uint32_t b) {
    m_colour = b | (g << 8u) | (r << 16u);
}

void Terminal::set_cursor(uint32_t x, uint32_t y) {
    // TODO: Bounds checking.
    m_cursor_x = x;
    m_cursor_y = y;
}

void Terminal::set_dirty() {
    for (auto &line : m_lines) {
        line.set_dirty(true);
    }
}
