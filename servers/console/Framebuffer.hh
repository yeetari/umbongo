#pragma once

#include <core/File.hh>
#include <kernel/SyscallTypes.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>

class Framebuffer {
    core::File m_file;
    FramebufferInfo m_info{};
    uint32 *m_back_buffer{nullptr};
    uint32 *m_front_buffer{nullptr};
    bool m_dirty{false};

public:
    explicit Framebuffer(ustd::StringView path);

    void clear();
    void clear_region(uint32 x, uint32 y, uint32 width, uint32 height);
    void set(uint32 x, uint32 y, uint32 colour);
    void swap_buffers();

    uint32 width() const { return m_info.width; }
    uint32 height() const { return m_info.height; }
};
