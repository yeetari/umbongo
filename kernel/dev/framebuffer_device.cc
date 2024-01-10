#include <kernel/dev/framebuffer_device.hh>

#include <kernel/api/types.h>
#include <kernel/error.hh>
#include <kernel/mem/address_space.hh>
#include <kernel/mem/region.hh>
#include <kernel/mem/vm_object.hh>
#include <kernel/sys_result.hh>
#include <ustd/try.hh>
#include <ustd/types.hh>

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

uintptr_t FramebufferDevice::mmap(AddressSpace &address_space) const {
    const size_t size = m_pitch * m_height;
    auto vm_object = VmObject::create_physical(m_base, size);
    auto &region = EXPECT(address_space.allocate_anywhere(size, RegionAccess::Writable | RegionAccess::UserAccessible));
    region.map(ustd::move(vm_object));
    return region.base();
}

} // namespace kernel
