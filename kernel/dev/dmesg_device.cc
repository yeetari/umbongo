#include <kernel/dev/dmesg_device.hh>

#include <boot/boot_info.hh>
#include <kernel/sys_result.hh>
#include <ustd/numeric.hh>
#include <ustd/ring_buffer.hh>
#include <ustd/span.hh>
#include <ustd/types.hh>
#include <ustd/utility.hh>

namespace kernel {
namespace {

ustd::RingBuffer<char, 128_KiB> *s_buffer;

} // namespace

void DmesgDevice::early_initialise(BootInfo *boot_info) {
    s_buffer = new (boot_info->dmesg_area) ustd::RingBuffer<char, 128_KiB>;
}

void DmesgDevice::initialise() {
    (new DmesgDevice)->leak_ref();
}

void DmesgDevice::put_char(char ch) {
    s_buffer->enqueue(ch);
}

SysResult<size_t> DmesgDevice::read(ustd::Span<void> data, size_t offset) {
    const auto size = ustd::min(data.size(), s_buffer->size() - offset);
    __builtin_memcpy(data.data(), &(*s_buffer)[offset], size);
    return size;
}

} // namespace kernel
