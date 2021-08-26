#include <kernel/devices/FramebufferDevice.hh>

#include <kernel/Font.hh>
#include <kernel/SysError.hh>
#include <kernel/SysResult.hh>
#include <kernel/SyscallTypes.hh>
#include <kernel/cpu/InterruptDisabler.hh>
#include <kernel/mem/Region.hh>
#include <kernel/mem/VirtSpace.hh>
#include <ustd/Memory.hh>
#include <ustd/Types.hh>

extern bool g_console_enabled;

SyscallResult FramebufferDevice::ioctl(IoctlRequest request, void *arg) {
    switch (request) {
    case IoctlRequest::FramebufferClear:
        g_console_enabled = false;
        memset(reinterpret_cast<uint32 *>(m_base), 0, m_pitch * m_height);
        return 0;
    case IoctlRequest::FramebufferGetInfo: {
        FramebufferInfo info{
            .width = m_width,
            .height = m_height,
        };
        memcpy(arg, &info, sizeof(FramebufferInfo));
        return 0;
    }
    case IoctlRequest::TerminalGetSize: {
        // TODO: Console server should manage this.
        TerminalSize size{
            .column_count = m_width / g_font.advance(),
            .row_count = m_height / g_font.line_height(),
        };
        memcpy(arg, &size, sizeof(TerminalSize));
        return 0;
    }
    default:
        return SysError::Invalid;
    }
}

uintptr FramebufferDevice::mmap(VirtSpace &virt_space) {
    // TODO: InterruptDisabler shouldn't be needed.
    InterruptDisabler disabler;
    const usize size = m_pitch * m_height;
    auto &region = virt_space.allocate_region(size, RegionAccess::Writable | RegionAccess::UserAccessible, m_base);
    return region.base();
}
