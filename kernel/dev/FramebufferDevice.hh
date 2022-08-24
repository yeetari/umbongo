#pragma once

#include <kernel/SysResult.hh>
#include <kernel/SyscallTypes.hh>
#include <kernel/dev/Device.hh>
#include <ustd/Types.hh>

namespace kernel {

class VirtSpace;

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
    SyscallResult ioctl(IoctlRequest request, void *arg) override;
    uintptr_t mmap(VirtSpace &virt_space) const override;
};

} // namespace kernel
