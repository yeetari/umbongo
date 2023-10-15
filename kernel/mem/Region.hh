#pragma once

#include <kernel/mem/PhysicalPage.hh>
#include <ustd/Optional.hh> // IWYU pragma: keep
#include <ustd/SharedPtr.hh>
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

namespace kernel {

class VirtSpace;

enum class RegionAccess : uint8_t {
    Writable = 1u << 0u,
    Executable = 1u << 1u,
    UserAccessible = 1u << 2u,
    Uncacheable = 1u << 3u,
    Global = 1u << 4u,
};

class Region {
    uintptr_t m_base;
    size_t m_size;
    RegionAccess m_access;
    bool m_free;
    ustd::Vector<ustd::SharedPtr<PhysicalPage>> m_physical_pages;

public:
    Region(uintptr_t base, size_t size, RegionAccess access, bool free, ustd::Optional<uintptr_t> phys_base);
    Region(const Region &) = default;
    Region(Region &&) = default;
    ~Region() = default;

    Region &operator=(const Region &) = delete;
    Region &operator=(Region &&) = delete;

    void map(VirtSpace *virt_space) const;
    void set_base(uintptr_t base) { m_base = base; }
    void set_size(size_t size) { m_size = size; }

    uintptr_t base() const { return m_base; }
    size_t size() const { return m_size; }
    RegionAccess access() const { return m_access; }
    bool free() const { return m_free; }
    const auto &physical_pages() const { return m_physical_pages; }
};

inline constexpr RegionAccess operator&(RegionAccess a, RegionAccess b) {
    return static_cast<RegionAccess>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

inline constexpr RegionAccess operator|(RegionAccess a, RegionAccess b) {
    return static_cast<RegionAccess>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline constexpr RegionAccess &operator|=(RegionAccess &a, RegionAccess b) {
    return a = (a | b);
}

} // namespace kernel
