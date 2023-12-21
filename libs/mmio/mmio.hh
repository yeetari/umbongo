#pragma once

#include <core/time.hh>
#include <system/error.h>
#include <system/syscall.hh>
#include <system/system.h>
#include <ustd/algorithm.hh>
#include <ustd/result.hh>
#include <ustd/try.hh>
#include <ustd/types.hh>

namespace mmio {

template <typename T>
ustd::Result<T *, ub_error_t> alloc_dma() {
    constexpr auto prot = UB_MEMORY_PROT_WRITE | UB_MEMORY_PROT_UNCACHEABLE;
    auto *region = TRY(system::syscall<T *>(UB_SYS_allocate_region, sizeof(T), prot));
    return &(*region = T{});
}

template <typename T>
ustd::Result<T *, ub_error_t> alloc_dma_array(size_t size) {
    if (size == 0) {
        return nullptr;
    }
    constexpr auto prot = UB_MEMORY_PROT_WRITE | UB_MEMORY_PROT_UNCACHEABLE;
    auto *region = TRY(system::syscall<T *>(UB_SYS_allocate_region, size * sizeof(T), prot));
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

inline ustd::Result<uintptr_t, ub_error_t> virt_to_phys(const void *virt) {
    return virt != nullptr ? TRY(system::syscall(UB_SYS_virt_to_phys, virt)) : 0ul;
}

} // namespace mmio
