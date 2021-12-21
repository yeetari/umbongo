#include <kernel/devices/FramebufferDevice.hh>

#include <kernel/SysError.hh>
#include <kernel/SysResult.hh>
#include <kernel/SyscallTypes.hh>
#include <kernel/mem/Region.hh>
#include <kernel/mem/VirtSpace.hh>
#include <ustd/Types.hh>

extern bool g_console_enabled;

SyscallResult FramebufferDevice::ioctl(IoctlRequest request, void *arg) {
    switch (request) {
    case IoctlRequest::FramebufferGetInfo: {
        g_console_enabled = false;
        FramebufferInfo info{
            .size = m_pitch * m_height,
            .width = m_width,
            .height = m_height,
        };
        __builtin_memcpy(arg, &info, sizeof(FramebufferInfo));
        return 0;
    }
    default:
        return SysError::Invalid;
    }
}

uintptr FramebufferDevice::mmap(VirtSpace &virt_space) {
    const usize size = m_pitch * m_height;
    auto &region = virt_space.allocate_region(size, RegionAccess::Writable | RegionAccess::UserAccessible, m_base);
    return region.base();
}
