#pragma once

#include <kernel/SysResult.hh>
#include <kernel/SyscallTypes.hh>
#include <kernel/dev/Device.hh>
#include <ustd/Array.hh>
#include <ustd/Span.hh> // IWYU pragma: keep
#include <ustd/Types.hh>

namespace kernel {

class VirtSpace;

} // namespace kernel

namespace kernel::pci {

struct Bar {
    uintptr address;
    usize size;
};

class Function final : public Device {
    const uint16 m_segment;
    const uint8 m_bus;
    const uint8 m_device;
    const uint8 m_function;
    uintptr m_address;
    uint32 m_class_info;
    uint16 m_vendor_id;
    uint16 m_device_id;
    ustd::Array<Bar, 6> m_bars{};
    bool m_interrupt_pending{false};

    template <typename F>
    void enumerate_capabilities(F callback) const;
    template <typename T>
    T read_config(uint32 offset) const;
    template <typename T>
    void write_config(uint32 offset, T value);

public:
    Function(uintptr segment_base, uint16 segment, uint8 bus, uint8 device, uint8 function);

    bool read_would_block(usize) const override { return false; }
    bool write_would_block(usize) const override { return !m_interrupt_pending; }
    SyscallResult ioctl(IoctlRequest request, void *arg) override;
    uintptr mmap(VirtSpace &virt_space) const override;
    SysResult<usize> read(ustd::Span<void> data, usize offset) override;
};

} // namespace kernel::pci
