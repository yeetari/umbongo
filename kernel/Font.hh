#pragma once

#include <ustd/Types.hh>

struct FontGlyph {
    const char ch;
    const uint32 width;
    const uint32 height;
    const uint32 left;
    const uint32 top;
    const uint8 *bitmap;
};

class Font {
    const char *m_name;
    const char *m_style;
    const uint32 m_advance;
    const uint32 m_ascender;
    const uint32 m_line_height;
    const FontGlyph *m_glyph_array;
    const usize m_glyph_count;

public:
    constexpr Font(const char *name, const char *style, uint32 advance, uint32 ascender, uint32 line_height,
                   const FontGlyph *glyph_array, usize glyph_count)
        : m_name(name), m_style(style), m_advance(advance), m_ascender(ascender), m_line_height(line_height),
          m_glyph_array(glyph_array), m_glyph_count(glyph_count) {}

    const FontGlyph *glyph(char ch) const;

    const char *name() const { return m_name; }
    const char *style() const { return m_style; }
    uint32 advance() const { return m_advance; }
    uint32 ascender() const { return m_ascender; }
    uint32 line_height() const { return m_line_height; }
};

extern const Font g_font;
