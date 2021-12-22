#pragma once

#include <ustd/Shareable.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Types.hh>

namespace kernel {

enum class PhysicalPageSize : uint8 {
    Normal,
    Large,
    Huge,
};

class PhysicalPage : public ustd::Shareable<PhysicalPage> {
    uintptr m_phys{0};

public:
    static ustd::SharedPtr<PhysicalPage> allocate(PhysicalPageSize size);
    static ustd::SharedPtr<PhysicalPage> create(uintptr phys, PhysicalPageSize size);

    // Maximum physical address size permitted in AMD64 is 52-bits, meaning we can use safely use 8 for the size enum
    // and allocated flag.
    PhysicalPage(uintptr phys, PhysicalPageSize size, bool allocated)
        : m_phys(phys | (static_cast<uint64>(size) << 56u) | (allocated ? (1ul << 63u) : 0u)) {}
    PhysicalPage(const PhysicalPage &) = delete;
    PhysicalPage(PhysicalPage &&) = delete;
    ~PhysicalPage();

    PhysicalPage &operator=(const PhysicalPage &) = delete;
    PhysicalPage &operator=(PhysicalPage &&) = delete;

    uintptr phys() const { return m_phys & 0xfffffffffffffful; }
    PhysicalPageSize size() const { return static_cast<PhysicalPageSize>((m_phys >> 56u) & 0xfu); }
    bool allocated() const { return (m_phys & (1ul << 63u)) == (1ul << 63u); }
};

} // namespace kernel
