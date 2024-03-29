#include "framebuffer.hh"

#include <core/file.hh>
#include <system/syscall.hh>
#include <ustd/assert.hh>
#include <ustd/string_view.hh>
#include <ustd/try.hh>
#include <ustd/types.hh>

Framebuffer::Framebuffer(ustd::StringView path) : m_file(EXPECT(core::File::open(path))) {
    EXPECT(m_file.ioctl(UB_IOCTL_REQUEST_FB_GET_INFO, &m_info));
    m_back_buffer = EXPECT(system::syscall<uint32_t *>(UB_SYS_allocate_region, m_info.size, UB_MEMORY_PROT_WRITE));
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
