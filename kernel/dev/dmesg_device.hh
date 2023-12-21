#pragma once

#include <kernel/dev/device.hh>
#include <kernel/sys_result.hh>
#include <ustd/span.hh> // IWYU pragma: keep
#include <ustd/types.hh>

struct BootInfo;

namespace kernel {

class DmesgDevice final : public Device {
public:
    static void early_initialise(BootInfo *boot_info);
    static void initialise();
    static void put_char(char ch);

    DmesgDevice() : Device("klog") {}

    bool read_would_block(size_t) const override { return false; }
    bool write_would_block(size_t) const override { return false; }
    SysResult<size_t> read(ustd::Span<void> data, size_t offset) override;
};

} // namespace kernel
