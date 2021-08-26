#pragma once

#include <kernel/SysResult.hh>
#include <kernel/SyscallTypes.hh>
#include <kernel/devices/Device.hh>
#include <ustd/Types.hh>

class VirtSpace;

class FramebufferDevice final : public Device {
    uintptr m_base;
    uint32 m_width;
    uint32 m_height;
    uint64 m_pitch;

public:
    explicit FramebufferDevice(uintptr base, uint32 width, uint32 height, uint64 pitch)
        : m_base(base), m_width(width), m_height(height), m_pitch(pitch) {}

    bool can_read() override { return true; }
    bool can_write() override { return true; }
    SyscallResult ioctl(IoctlRequest request, void *arg) override;
    uintptr mmap(VirtSpace &virt_space) override;

    const char *name() const override { return "fb"; }
};
