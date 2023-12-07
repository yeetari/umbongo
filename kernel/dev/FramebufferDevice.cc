#include <kernel/dev/FramebufferDevice.hh>

#include <kernel/Error.hh>
#include <kernel/SysResult.hh>
#include <kernel/api/Types.h>
#include <kernel/mem/Region.hh>
#include <kernel/mem/VirtSpace.hh>
#include <ustd/Types.hh>

namespace kernel {

extern bool g_console_enabled;

SyscallResult FramebufferDevice::ioctl(ub_ioctl_request_t request, void *arg) {
    switch (request) {
    case UB_IOCTL_REQUEST_FB_GET_INFO: {
        g_console_enabled = false;
        ub_fb_info_t info{
            .size = m_pitch * m_height,
            .width = m_width,
            .height = m_height,
        };
        __builtin_memcpy(arg, &info, sizeof(ub_fb_info_t));
        return 0;
    }
    default:
        return Error::Invalid;
    }
}

uintptr_t FramebufferDevice::mmap(VirtSpace &virt_space) const {
    const size_t size = m_pitch * m_height;
    auto &region = virt_space.allocate_region(size, RegionAccess::Writable | RegionAccess::UserAccessible, m_base);
    return region.base();
}

} // namespace kernel
