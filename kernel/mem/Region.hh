#pragma once

#include <kernel/mem/PhysicalPage.hh>
#include <ustd/Optional.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

namespace kernel {

class VirtSpace;

enum class RegionAccess : uint8 {
    Writable = 1u << 0u,
    Executable = 1u << 1u,
    UserAccessible = 1u << 2u,
    Uncacheable = 1u << 3u,
};

class Region {
    uintptr m_base;
    usize m_size;
    RegionAccess m_access;
    bool m_free;
    ustd::Vector<ustd::SharedPtr<PhysicalPage>> m_physical_pages;

public:
    Region(uintptr base, usize size, RegionAccess access, bool free, ustd::Optional<uintptr> phys_base);
    Region(const Region &) = default;
    Region(Region &&) noexcept = default;
    ~Region() = default;

    Region &operator=(const Region &) = delete;
    Region &operator=(Region &&) = delete;

    void map(VirtSpace *virt_space) const;
    void set_base(uintptr base) { m_base = base; }
    void set_size(usize size) { m_size = size; }

    uintptr base() const { return m_base; }
    usize size() const { return m_size; }
    RegionAccess access() const { return m_access; }
    bool free() const { return m_free; }
    const auto &physical_pages() const { return m_physical_pages; }
};

inline constexpr RegionAccess operator&(RegionAccess a, RegionAccess b) {
    return static_cast<RegionAccess>(static_cast<uint8>(a) & static_cast<uint8>(b));
}

inline constexpr RegionAccess operator|(RegionAccess a, RegionAccess b) {
    return static_cast<RegionAccess>(static_cast<uint8>(a) | static_cast<uint8>(b));
}

inline constexpr RegionAccess &operator|=(RegionAccess &a, RegionAccess b) {
    return a = (a | b);
}

} // namespace kernel
