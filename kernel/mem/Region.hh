#pragma once

#include <kernel/HandleKind.hh>
#include <kernel/mem/VirtualRange.hh>
#include <ustd/Optional.hh>
#include <ustd/Shareable.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/SplayTree.hh>
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

namespace kernel {

class VirtSpace;
class VmObject;

enum class RegionAccess : uint8_t {
    None = 0,
    Writable = 1u << 0u,
    Executable = 1u << 1u,
    UserAccessible = 1u << 2u,
    Uncacheable = 1u << 3u,
    Global = 1u << 4u,
};

class Region : public ustd::IntrusiveTreeNode<Region>, public ustd::Shareable<Region> {
    friend VirtSpace;

private:
    VirtSpace &m_virt_space;
    const VirtualRange m_range;
    const RegionAccess m_access;
    ustd::SharedPtr<VmObject> m_vm_object;

    Region(VirtSpace &virt_space, VirtualRange range, RegionAccess access);

public:
    static constexpr auto k_handle_kind = HandleKind::Region;
    Region(const Region &) = delete;
    Region(Region &&) = delete;
    ~Region();

    Region &operator=(const Region &) = delete;
    Region &operator=(Region &&) = delete;

    void map(ustd::SharedPtr<VmObject> &&vm_object);
    void unmap_if_needed();

    uintptr_t base() const { return m_range.base(); }
    VirtualRange range() const { return m_range; }
    RegionAccess access() const { return m_access; }
    ustd::SharedPtr<VmObject> vm_object() const;
};

using RegionTree = ustd::SplayTree<uintptr_t, Region, &Region::base>;

inline constexpr RegionAccess operator&(RegionAccess lhs, RegionAccess rhs) {
    return static_cast<RegionAccess>(ustd::to_underlying(lhs) & ustd::to_underlying(rhs));
}

inline constexpr RegionAccess operator|(RegionAccess lhs, RegionAccess rhs) {
    return static_cast<RegionAccess>(ustd::to_underlying(lhs) | ustd::to_underlying(rhs));
}

} // namespace kernel
