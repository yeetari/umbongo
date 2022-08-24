#pragma once

#include <ustd/Types.hh>

struct FontGlyph {
    const char ch;
    const uint32_t width;
    const uint32_t height;
    const int32_t left;
    const int32_t top;
    const uint8_t *bitmap;
};

class Font {
    const char *m_name;
    const char *m_style;
    const uint32_t m_advance;
    const uint32_t m_ascender;
    const uint32_t m_line_height;
    const FontGlyph *m_glyph_array;
    const size_t m_glyph_count;

public:
    constexpr Font(const char *name, const char *style, uint32_t advance, uint32_t ascender, uint32_t line_height,
                   const FontGlyph *glyph_array, size_t glyph_count)
        : m_name(name), m_style(style), m_advance(advance), m_ascender(ascender), m_line_height(line_height),
          m_glyph_array(glyph_array), m_glyph_count(glyph_count) {}

    const FontGlyph *glyph(char ch) const;

    const char *name() const { return m_name; }
    const char *style() const { return m_style; }
    uint32_t advance() const { return m_advance; }
    uint32_t ascender() const { return m_ascender; }
    uint32_t line_height() const { return m_line_height; }
};

extern const Font g_font;
