#pragma once

#include <kernel/arch/paging.hh>
#include <kernel/mem/region.hh>
#include <kernel/spin_lock.hh>
#include <kernel/sys_result.hh>
#include <ustd/rb_tree.hh>
#include <ustd/splay_tree.hh>
#include <ustd/unique_ptr.hh>
#include <ustd/vector.hh>

namespace kernel {

class Process;

using RegionTree = ustd::SplayTree<uintptr_t, Region, &Region::base>;

class AddressSpace {
    friend Region;

private:
    Process &m_process;
    ustd::UniquePtr<Pml4> m_pml4;
    RegionTree m_region_tree;
    mutable SpinLock m_lock;

    // TODO: This storage is only temporary as regions will become shared objects.
    ustd::Vector<ustd::UniquePtr<Region>> m_regions;

    void detree_region(Region *region);
    void map_4KiB(uintptr_t virt, uintptr_t phys, PageFlags flags);
    void map_2MiB(uintptr_t virt, uintptr_t phys, PageFlags flags);
    void map_1GiB(uintptr_t virt, uintptr_t phys, PageFlags flags);
    void unmap_4KiB(uintptr_t virt);
    void unmap_2MiB(uintptr_t virt);
    void unmap_1GiB(uintptr_t virt);

    SysResult<VirtualRange> allocate_range_anywhere(size_t size);
    SysResult<VirtualRange> allocate_range_specific(VirtualRange range);
    Region &new_region(VirtualRange range, RegionAccess access);

public:
    explicit AddressSpace(Process &process);
    AddressSpace(const AddressSpace &) = delete;
    AddressSpace(AddressSpace &&) = delete;
    ~AddressSpace();

    AddressSpace &operator=(const AddressSpace &) = delete;
    AddressSpace &operator=(AddressSpace &&) = delete;

    SysResult<Region &> allocate_anywhere(size_t size, RegionAccess access);
    SysResult<Region &> allocate_specific(VirtualRange range, RegionAccess access);
    SysResult<uintptr_t> virt_to_phys(uintptr_t virt);

    Process &process() const { return m_process; }
    void *pml4_ptr() const { return m_pml4.ptr(); }
};

} // namespace kernel
