#pragma once

// IWYU pragma: private, include <kernel/arch/cpu.hh>

namespace kernel::arch {

inline void cpu_relax() {
    asm volatile("pause" ::: "memory");
}

} // namespace kernel::arch
