#pragma once

#include <core/Process.hh>
#include <kernel/SysError.hh>
#include <kernel/Syscall.hh>
#include <ustd/Algorithm.hh>
#include <ustd/Result.hh>
#include <ustd/Types.hh>

namespace mmio {

template <typename T>
ustd::Result<T *, SysError> alloc_dma() {
    constexpr auto prot = MemoryProt::Write | MemoryProt::Uncacheable;
    auto *region = TRY(Syscall::invoke<T *>(Syscall::allocate_region, sizeof(T), prot));
    return &(*region = T{});
}

template <typename T>
ustd::Result<T *, SysError> alloc_dma_array(usize size) {
    if (size == 0) {
        return nullptr;
    }
    constexpr auto prot = MemoryProt::Write | MemoryProt::Uncacheable;
    auto *region = TRY(Syscall::invoke<T *>(Syscall::allocate_region, size * sizeof(T), prot));
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
bool wait_timeout(const volatile T &value, T mask, T desired, usize timeout) {
    for (; (value & mask) != desired; timeout--) {
        if (timeout == 0) {
            return false;
        }
        core::sleep(1000000ul);
    }
    return true;
}

inline ustd::Result<uintptr, SysError> virt_to_phys(const void *virt) {
    return virt != nullptr ? TRY(Syscall::invoke(Syscall::virt_to_phys, virt)) : 0ul;
}

} // namespace mmio
