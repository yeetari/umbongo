#pragma once

#include <ustd/types.hh>

namespace kernel::arch {

class CpuId {
    uint32_t m_eax{0};
    uint32_t m_ebx{0};
    uint32_t m_ecx{0};
    uint32_t m_edx{0};

public:
    explicit CpuId(uint32_t function, uint32_t sub_function = 0);

    uint32_t eax() const { return m_eax; }
    uint32_t ebx() const { return m_ebx; }
    uint32_t ecx() const { return m_ecx; }
    uint32_t edx() const { return m_edx; }
};

inline CpuId::CpuId(uint32_t function, uint32_t sub_function) {
    asm volatile("cpuid" : "=a"(m_eax), "=b"(m_ebx), "=c"(m_ecx), "=d"(m_edx) : "a"(function), "c"(sub_function));
}

} // namespace kernel::arch
