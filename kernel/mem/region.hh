#pragma once

#include <kernel/mem/virtual_range.hh>
#include <ustd/shared_ptr.hh>
#include <ustd/splay_tree.hh> // TODO

namespace kernel {

class AddressSpace;
class VmObject;

enum class RegionAccess : uint32_t {
    None = 0,
    Writable = 1u << 0u,
    Executable = 1u << 1u,
    UserAccessible = 1u << 2u,
    Uncacheable = 1u << 3u,
    Global = 1u << 4u,
};

class Region : public ustd::IntrusiveTreeNode<Region> {
    friend AddressSpace;

private:
    AddressSpace &m_address_space;
    const VirtualRange m_range;
    const RegionAccess m_access;
    ustd::SharedPtr<VmObject> m_vm_object;

    Region(AddressSpace &, VirtualRange, RegionAccess);

public:
    Region(const Region &) = delete;
    Region(Region &&) = delete;
    ~Region();

    Region &operator=(const Region &) = delete;
    Region &operator=(Region &&) = delete;

    void map(ustd::SharedPtr<VmObject> &&vm_object);
    void unmap_if_needed();

    uintptr_t base() const { return m_range.base(); }
    AddressSpace &address_space() const { return m_address_space; }
    VirtualRange range() const { return m_range; }
    RegionAccess access() const { return m_access; }
    ustd::SharedPtr<VmObject> vm_object() const;
};

inline constexpr RegionAccess operator&(RegionAccess lhs, RegionAccess rhs) {
    return static_cast<RegionAccess>(ustd::to_underlying(lhs) & ustd::to_underlying(rhs));
}

inline constexpr RegionAccess operator|(RegionAccess lhs, RegionAccess rhs) {
    return static_cast<RegionAccess>(ustd::to_underlying(lhs) | ustd::to_underlying(rhs));
}

inline constexpr RegionAccess &operator|=(RegionAccess &lhs, RegionAccess rhs) {
    return lhs = (lhs | rhs);
}

} // namespace kernel
