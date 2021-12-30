#pragma once

#include <kernel/SysResult.hh>
#include <kernel/dev/Device.hh>
#include <ustd/Span.hh> // IWYU pragma: keep
#include <ustd/Types.hh>

struct BootInfo;

namespace kernel {

class DmesgDevice final : public Device {
public:
    static void early_initialise(BootInfo *boot_info);
    static void initialise();
    static void put_char(char ch);

    DmesgDevice() : Device("klog") {}

    bool read_would_block(usize) const override { return false; }
    bool write_would_block(usize) const override { return false; }
    SysResult<usize> read(ustd::Span<void> data, usize offset) override;
};

} // namespace kernel
