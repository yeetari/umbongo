#include "Framebuffer.hh"

#include <core/File.hh>
#include <core/Syscall.hh>
#include <ustd/Assert.hh>
#include <ustd/StringView.hh>
#include <ustd/Try.hh>
#include <ustd/Types.hh>

Framebuffer::Framebuffer(ustd::StringView path) : m_file(EXPECT(core::File::open(path))) {
    EXPECT(m_file.ioctl(kernel::IoctlRequest::FramebufferGetInfo, &m_info));
    m_back_buffer = EXPECT(core::syscall<uint32_t *>(Syscall::allocate_region, m_info.size, kernel::MemoryProt::Write));
    m_front_buffer = EXPECT(m_file.mmap<uint32_t>());
}

void Framebuffer::clear() {
    __builtin_memset(m_back_buffer, 0, m_info.size);
    m_dirty = true;
}

void Framebuffer::clear_region(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    for (uint32_t y1 = y; y1 < y + height; y1++) {
        __builtin_memset(&m_back_buffer[y1 * m_info.width + x], 0, width * sizeof(uint32_t));
    }
    m_dirty = true;
}

void Framebuffer::set(uint32_t x, uint32_t y, uint32_t colour) {
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
