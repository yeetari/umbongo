#include "Framebuffer.hh"

#include <core/File.hh>
#include <kernel/Syscall.hh>
#include <kernel/SyscallTypes.hh>
#include <ustd/Assert.hh>
#include <ustd/Memory.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>

Framebuffer::Framebuffer(StringView path) : m_file(path) {
    ENSURE(m_file, "Failed to open framebuffer");
    Syscall::invoke(Syscall::ioctl, m_file.fd(), IoctlRequest::FramebufferGetInfo, &m_info);
    m_pixels = Syscall::invoke<uint32 *>(Syscall::mmap, m_file.fd());
}

void Framebuffer::clear() const {
    Syscall::invoke(Syscall::ioctl, m_file.fd(), IoctlRequest::FramebufferClear);
}

void Framebuffer::clear_region(uint32 x, uint32 y, uint32 width, uint32 height) const {
    for (uint32 y1 = y; y1 < y + height; y1++) {
        memset(&m_pixels[y1 * m_info.width + x], 0, width * sizeof(uint32));
    }
}

void Framebuffer::set(uint32 x, uint32 y, uint32 colour) const {
    ASSERT_PEDANTIC(x < m_info.width);
    ASSERT_PEDANTIC(y < m_info.height);
    m_pixels[y * m_info.width + x] = colour;
}
