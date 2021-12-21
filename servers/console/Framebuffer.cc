#include "Framebuffer.hh"

#include <core/File.hh>
#include <kernel/Syscall.hh>
#include <kernel/SyscallTypes.hh>
#include <ustd/Assert.hh>
#include <ustd/Result.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>

Framebuffer::Framebuffer(ustd::StringView path) : m_file(EXPECT(core::File::open(path))) {
    Syscall::invoke(Syscall::ioctl, m_file.fd(), IoctlRequest::FramebufferGetInfo, &m_info);
    m_back_buffer = Syscall::invoke<uint32 *>(Syscall::allocate_region, m_info.size, MemoryProt::Write);
    m_front_buffer = Syscall::invoke<uint32 *>(Syscall::mmap, m_file.fd());
}

void Framebuffer::clear() {
    __builtin_memset(m_back_buffer, 0, m_info.size);
    m_dirty = true;
}

void Framebuffer::clear_region(uint32 x, uint32 y, uint32 width, uint32 height) {
    for (uint32 y1 = y; y1 < y + height; y1++) {
        __builtin_memset(&m_back_buffer[y1 * m_info.width + x], 0, width * sizeof(uint32));
    }
    m_dirty = true;
}

void Framebuffer::set(uint32 x, uint32 y, uint32 colour) {
    ASSERT_PEDANTIC(x < m_info.width);
    ASSERT_PEDANTIC(y < m_info.height);
    m_back_buffer[y * m_info.width + x] = colour;
    m_dirty = true;
}

void Framebuffer::swap_buffers() {
    if (!m_dirty) {
        return;
    }
    m_dirty = false;
    __builtin_memcpy(m_front_buffer, m_back_buffer, m_info.size);
}
