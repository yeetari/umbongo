#pragma once

#include <kernel/api/types.h>
#include <kernel/dev/device.hh>
#include <kernel/sys_result.hh>
#include <ustd/types.hh>

namespace kernel {

class AddressSpace;

class FramebufferDevice final : public Device {
    const uintptr_t m_base;
    const uint32_t m_width;
    const uint32_t m_height;
    const uint64_t m_pitch;

public:
    FramebufferDevice(uintptr_t base, uint32_t width, uint32_t height, uint64_t pitch)
        : Device("fb"), m_base(base), m_width(width), m_height(height), m_pitch(pitch) {}

    bool read_would_block(size_t) const override { return false; }
    bool write_would_block(size_t) const override { return false; }
    SyscallResult ioctl(ub_ioctl_request_t request, void *arg) override;
    uintptr_t mmap(AddressSpace &address_space) const override;
};

} // namespace kernel
