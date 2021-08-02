#include <kernel/devices/FramebufferDevice.hh>

#include <kernel/SysError.hh>
#include <kernel/SysResult.hh>
#include <kernel/SyscallTypes.hh>
#include <kernel/mem/Region.hh>
#include <kernel/mem/VirtSpace.hh>
#include <ustd/Memory.hh>
#include <ustd/Types.hh>

SysResult FramebufferDevice::ioctl(IoctlRequest request, void *arg) {
    switch (request) {
    case IoctlRequest::FramebufferClear:
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
    default:
        return SysError::Invalid;
    }
}

uintptr FramebufferDevice::mmap(VirtSpace &virt_space) {
    const usize size = m_pitch * m_height;
    auto &region = virt_space.allocate_region(size, RegionAccess::Writable | RegionAccess::UserAccessible, m_base);
    return region.base();
}
