#pragma once

#include <core/File.hh>
#include <kernel/SyscallTypes.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>

class Framebuffer {
    core::File m_file;
    FramebufferInfo m_info{};
    uint32 *m_pixels{nullptr};

public:
    explicit Framebuffer(StringView path);

    void clear() const;
    void clear_region(uint32 x, uint32 y, uint32 width, uint32 height) const;
    void set(uint32 x, uint32 y, uint32 colour) const;

    uint32 width() const { return m_info.width; }
    uint32 height() const { return m_info.height; }
};
