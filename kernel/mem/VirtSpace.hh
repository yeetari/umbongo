#pragma once

#include <kernel/SpinLock.hh>
#include <kernel/SysResult.hh>
#include <kernel/cpu/Paging.hh>
#include <kernel/mem/Region.hh>
#include <ustd/Array.hh>
#include <ustd/Optional.hh>
#include <ustd/Shareable.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/SplayTree.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>
#include <ustd/Vector.hh>

namespace kernel {

struct MemoryManager;
class VmObject;

class VirtSpace : public ustd::Shareable<VirtSpace> {
    USTD_ALLOW_MAKE_SHARED;
    friend MemoryManager;
    friend Region;

private:
    ustd::UniquePtr<Pml4> m_pml4;
    RegionTree m_region_tree;
    ustd::Vector<ustd::SharedPtr<Region>> m_inherited_regions;
    mutable SpinLock m_lock;

    VirtSpace();

    void detree_region(Region *region);
    void map_4KiB(uintptr_t virt, uintptr_t phys, PageFlags flags);
    void map_2MiB(uintptr_t virt, uintptr_t phys, PageFlags flags);
    void map_1GiB(uintptr_t virt, uintptr_t phys, PageFlags flags);
    void unmap_4KiB(uintptr_t virt);
    void unmap_2MiB(uintptr_t virt);
    void unmap_1GiB(uintptr_t virt);

public:
    VirtSpace(const VirtSpace &) = delete;
    VirtSpace(VirtSpace &&) = delete;
    ~VirtSpace();

    VirtSpace &operator=(const VirtSpace &) = delete;
    VirtSpace &operator=(VirtSpace &&) = delete;

    SysResult<ustd::SharedPtr<Region>> allocate_anywhere(RegionAccess access, size_t size);
    SysResult<ustd::SharedPtr<Region>> allocate_anywhere(RegionAccess access, ustd::SharedPtr<VmObject> vm_object);
    SysResult<ustd::SharedPtr<Region>> allocate_specific(uintptr_t base, size_t size, RegionAccess access);
    SysResult<ustd::SharedPtr<Region>> allocate_specific(uintptr_t base, RegionAccess access,
                                                         ustd::SharedPtr<VmObject> vm_object);

    ustd::SharedPtr<VirtSpace> clone() const;
};

} // namespace kernel
