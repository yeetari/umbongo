#include <ft2build.h> // NOLINT
#include FT_FREETYPE_H

#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

class Face {
    FT_Face m_face{nullptr};

public:
    explicit Face(FT_Face face) : m_face(face) {}
    Face(const Face &) = delete;
    Face(Face &&) = delete;
    ~Face() { FT_Done_Face(m_face); }

    Face &operator=(const Face &) = delete;
    Face &operator=(Face &&) = delete;

    void set_pixel_sizes(FT_UInt width, FT_UInt height) {
        if (FT_Set_Pixel_Sizes(m_face, width, height) != 0) {
            throw std::runtime_error("Failed to set pixel sizes!");
        }
    }

    FT_UInt get_char_index(FT_ULong code) {
        FT_UInt index = FT_Get_Char_Index(m_face, code);
        if (index == 0) {
            throw std::runtime_error("Character not supported!");
        }
        return index;
    }

    FT_GlyphSlot load_glyph(FT_UInt index, FT_Int32 flags) {
        if (FT_Load_Glyph(m_face, index, flags) != 0) {
            throw std::runtime_error("Failed to load glyph!");
        }
        return m_face->glyph;
    }

    void render_glyph(FT_GlyphSlot glyph, FT_Render_Mode mode) {
        if (FT_Render_Glyph(glyph, mode) != 0) {
            throw std::runtime_error("Failed to render glyph!");
        }
    }

    const char *family_name() const { return m_face->family_name; }
    const char *style_name() const { return m_face->style_name; }
    FT_Vector advance() const { return m_face->glyph->advance; }
    FT_Pos ascender() const { return m_face->size->metrics.ascender; }
    FT_Pos line_height() const { return m_face->size->metrics.ascender - m_face->size->metrics.descender; }
};

class FreeType {
    FT_Library m_lib{nullptr};

public:
    FreeType() {
        if (FT_Init_FreeType(&m_lib) != 0) {
            throw std::runtime_error("Failed to initialise FreeType!");
        }
    }
    FreeType(const FreeType &) = delete;
    FreeType(FreeType &&) = delete;
    ~FreeType() { FT_Done_FreeType(m_lib); }

    FreeType &operator=(const FreeType &) = delete;
    FreeType &operator=(FreeType &&) = delete;

    Face new_face(const char *path, FT_Long face_index) {
        FT_Face face = nullptr;
        if (FT_New_Face(m_lib, path, face_index, &face) != 0) {
            throw std::runtime_error("Failed to open font!");
        }
        return Face(face);
    }
};

int main(int argc, char **argv) {
    if (argc != 4) {
        std::cout << "Usage: " << argv[0] << " <input> <output> <size>\n";
        return 1;
    }

    std::ofstream output_file(argv[2]);
    if (!output_file) {
        throw std::runtime_error("Failed to open output file!");
    }

    output_file << "#include <kernel/Font.hh>\n";
    output_file << "#include <ustd/Array.hh>\n\n";

    FreeType freetype;
    auto face = freetype.new_face(argv[1], 0);
    face.set_pixel_sizes(0, std::atoi(argv[3]));

    std::string characters = " !\"#$%&\'()*+,-./"
                             "0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`"
                             "abcdefghijklmnopqrstuvwxyz{|}~";
    output_file << "namespace {\n\n";
    output_file << "Array<FontGlyph, " << characters.size() << "> s_glyphs{{\n";
    for (auto ch : characters) {
        FT_UInt char_index = face.get_char_index(ch);
        auto *glyph = face.load_glyph(char_index, FT_LOAD_DEFAULT);
        face.render_glyph(glyph, FT_RENDER_MODE_NORMAL);

        output_file << "    {\n        ";
        if (ch == '\'' || ch == '\"' || ch == '\\') {
            output_file << "'\\" << ch << "',\n";
        } else {
            output_file << "'" << ch << "',\n";
        }

        output_file << "        " << glyph->bitmap.width << ",\n";
        output_file << "        " << glyph->bitmap.rows << ",\n";
        output_file << "        " << glyph->bitmap_left << ",\n";
        output_file << "        " << glyph->bitmap_top << ",\n";
        output_file << "        ";
        if (glyph->bitmap.width == 0 || glyph->bitmap.rows == 0) {
            output_file << "reinterpret_cast<const uint8 *>(0\n";
        } else {
            output_file << "reinterpret_cast<const uint8 *>(\"";
            for (unsigned int y = 0; y < glyph->bitmap.rows; y++) {
                for (unsigned int x = 0; x < glyph->bitmap.width; x++) {
                    output_file << "\\x" << std::setfill('0') << std::setw(2) << std::hex
                                << static_cast<unsigned int>(glyph->bitmap.buffer[y * glyph->bitmap.pitch + x]);
                    output_file << std::setw(0) << std::dec;
                }
                output_file << "\"\n";
                if (y + 1 < glyph->bitmap.rows) {
                    output_file << "        \"";
                }
            }
        }
        output_file << "    )},\n";
    }
    output_file << "}};\n\n";
    output_file << "} // namespace\n\n";

    output_file << "const Font g_font = {\n";
    output_file << "    \"" << face.family_name() << "\",\n";
    output_file << "    \"" << face.style_name() << "\",\n";
    output_file << "    " << (face.advance().x >> 6u) << ",\n";   // NOLINT
    output_file << "    " << (face.ascender() >> 6u) << ",\n";    // NOLINT
    output_file << "    " << (face.line_height() >> 6u) << ",\n"; // NOLINT
    output_file << "    s_glyphs.data(),\n";
    output_file << "    s_glyphs.size(),\n";
    output_file << "};\n\n";

    output_file << "const FontGlyph *Font::glyph(char ch) const {\n";
    output_file << "    for (usize i = 0; i < m_glyph_count; i++) {\n";
    output_file << "        if (m_glyph_array[i].ch == ch) {\n";
    output_file << "            return &m_glyph_array[i];\n";
    output_file << "        }\n";
    output_file << "    }\n";
    output_file << "    return glyph('?');\n";
    output_file << "}\n";
}
