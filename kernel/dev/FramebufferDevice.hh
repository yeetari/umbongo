#pragma once

#include <kernel/SysResult.hh>
#include <kernel/SyscallTypes.hh>
#include <kernel/dev/Device.hh>
#include <ustd/Types.hh>

namespace kernel {

class VirtSpace;

class FramebufferDevice final : public Device {
    const uintptr m_base;
    const uint32 m_width;
    const uint32 m_height;
    const uint64 m_pitch;

public:
    FramebufferDevice(uintptr base, uint32 width, uint32 height, uint64 pitch)
        : Device("fb"), m_base(base), m_width(width), m_height(height), m_pitch(pitch) {}

    bool read_would_block(usize) const override { return false; }
    bool write_would_block(usize) const override { return false; }
    SyscallResult ioctl(IoctlRequest request, void *arg) override;
    uintptr mmap(VirtSpace &virt_space) const override;
};

} // namespace kernel
