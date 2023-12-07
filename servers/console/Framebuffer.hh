#pragma once

#include <core/File.hh>
#include <system/System.h>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>

class Framebuffer {
    core::File m_file;
    ub_fb_info_t m_info{};
    uint32_t *m_back_buffer{nullptr};
    uint32_t *m_front_buffer{nullptr};
    bool m_dirty{false};

public:
    explicit Framebuffer(ustd::StringView path);

    void clear();
    void clear_region(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
    void set(uint32_t x, uint32_t y, uint32_t colour);
    void swap_buffers();

    uint32_t width() const { return m_info.width; }
    uint32_t height() const { return m_info.height; }
};
