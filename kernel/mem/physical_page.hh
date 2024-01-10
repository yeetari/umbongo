#pragma once

#include <ustd/types.hh>
#include <ustd/utility.hh>

namespace kernel {

enum class PhysicalPageSize : uint8_t {
    Normal,
    Large,
    Huge,
};

class PhysicalPage {
    uintptr_t m_phys;

public:
    static PhysicalPage allocate(PhysicalPageSize size);
    static PhysicalPage create(uintptr_t phys, PhysicalPageSize size);

    // Maximum physical address size permitted in AMD64 is 52-bits, meaning we can use safely use 8 for the size enum
    // and allocated flag.
    PhysicalPage(uintptr_t phys, PhysicalPageSize size, bool allocated)
        : m_phys(phys | (static_cast<uint64_t>(size) << 56u) | (allocated ? (1ul << 63u) : 0u)) {}
    PhysicalPage(const PhysicalPage &) = delete;
    PhysicalPage(PhysicalPage &&other) : m_phys(ustd::exchange(other.m_phys, 0u)) {}
    ~PhysicalPage();

    PhysicalPage &operator=(const PhysicalPage &) = delete;
    PhysicalPage &operator=(PhysicalPage &&) = delete;

    uintptr_t phys() const { return m_phys & 0xfffffffffffffful; }
    PhysicalPageSize size() const { return static_cast<PhysicalPageSize>((m_phys >> 56u) & 0xfu); }
    bool allocated() const { return (m_phys & (1ul << 63u)) != 0u; }
};

} // namespace kernel
