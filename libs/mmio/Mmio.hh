#pragma once

#include <core/Error.hh>
#include <core/Syscall.hh>
#include <core/Time.hh>
#include <ustd/Algorithm.hh>
#include <ustd/Result.hh>
#include <ustd/Try.hh>
#include <ustd/Types.hh>

namespace mmio {

template <typename T>
ustd::Result<T *, core::SysError> alloc_dma() {
    constexpr auto prot = kernel::MemoryProt::Write | kernel::MemoryProt::Uncacheable;
    auto *region = TRY(core::syscall<T *>(Syscall::allocate_region, sizeof(T), prot));
    return &(*region = T{});
}

template <typename T>
ustd::Result<T *, core::SysError> alloc_dma_array(size_t size) {
    if (size == 0) {
        return nullptr;
    }
    constexpr auto prot = kernel::MemoryProt::Write | kernel::MemoryProt::Uncacheable;
    auto *region = TRY(core::syscall<T *>(Syscall::allocate_region, size * sizeof(T), prot));
    ustd::fill_n(region, size, T{});
    return region;
}

template <typename T>
T read(const volatile T &value) {
    return value;
}

template <typename T>
void write(const volatile T &addr, T value) {
    const_cast<volatile T &>(addr) = value;
}

template <typename T>
bool wait_timeout(const volatile T &value, T mask, T desired, size_t timeout) {
    for (; (value & mask) != desired; timeout--) {
        if (timeout == 0) {
            return false;
        }
        core::sleep(1000000ul);
    }
    return true;
}

inline ustd::Result<uintptr_t, core::SysError> virt_to_phys(const void *virt) {
    return virt != nullptr ? TRY(core::syscall(Syscall::virt_to_phys, virt)) : 0ul;
}

} // namespace mmio
