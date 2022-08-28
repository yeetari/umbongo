#include <kernel/dev/DmesgDevice.hh>

#include <boot/BootInfo.hh>
#include <kernel/SysResult.hh>
#include <ustd/Numeric.hh>
#include <ustd/RingBuffer.hh>
#include <ustd/Span.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>

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
