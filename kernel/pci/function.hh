#pragma once

#include <kernel/api/types.h>
#include <kernel/dev/device.hh>
#include <kernel/sys_result.hh>
#include <ustd/array.hh>
#include <ustd/span.hh> // IWYU pragma: keep
#include <ustd/types.hh>

namespace kernel {

class AddressSpace;

} // namespace kernel

namespace kernel::pci {

struct Bar {
    uintptr_t address;
    size_t size;
};

class Function final : public Device {
    const uint16_t m_segment;
    const uint8_t m_bus;
    const uint8_t m_device;
    const uint8_t m_function;
    uintptr_t m_address;
    uint32_t m_class_info;
    uint16_t m_vendor_id;
    uint16_t m_device_id;
    ustd::Array<Bar, 6> m_bars{};
    bool m_interrupt_pending{false};

    template <typename F>
    void enumerate_capabilities(F callback) const;
    template <typename T>
    T read_config(uint32_t offset) const;
    template <typename T>
    void write_config(uint32_t offset, T value);

public:
    Function(uintptr_t segment_base, uint16_t segment, uint8_t bus, uint8_t device, uint8_t function);

    bool read_would_block(size_t) const override { return false; }
    bool write_would_block(size_t) const override { return !m_interrupt_pending; }
    SyscallResult ioctl(ub_ioctl_request_t request, void *arg) override;
    uintptr_t mmap(AddressSpace &address_space) const override;
    SysResult<size_t> read(ustd::Span<void> data, size_t offset) override;
};

} // namespace kernel::pci
