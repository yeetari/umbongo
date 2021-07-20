#pragma once

#include <ustd/Shareable.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Types.hh>

enum class PhysicalPageSize : uint8 {
    Normal,
    Huge,
};

class PhysicalPage : public Shareable<PhysicalPage> {
    uintptr m_phys{0};

public:
    static SharedPtr<PhysicalPage> allocate();
    static SharedPtr<PhysicalPage> create(uintptr phys, PhysicalPageSize size);

    // Maximum physical address size permitted in AMD64 is 52-bits, meaning we can use safely use 8 for the size enum.
    PhysicalPage(uintptr phys, PhysicalPageSize size) : m_phys(phys | (static_cast<uint64>(size) << 56u)) {}
    PhysicalPage(const PhysicalPage &) = delete;
    PhysicalPage(PhysicalPage &&) = delete;
    ~PhysicalPage();

    PhysicalPage &operator=(const PhysicalPage &) = delete;
    PhysicalPage &operator=(PhysicalPage &&) = delete;

    uintptr phys() const { return m_phys & 0xfffffffffffffful; }
    PhysicalPageSize size() const { return static_cast<PhysicalPageSize>((m_phys >> 56u) & 0xffu); }
};
