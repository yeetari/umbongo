#pragma once

#include <kernel/SpinLock.hh>
#include <kernel/cpu/Paging.hh>
#include <kernel/mem/Region.hh>
#include <ustd/Optional.hh>
#include <ustd/Shareable.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>
#include <ustd/Vector.hh>

namespace kernel {

struct MemoryManager;

class VirtSpace : public ustd::Shareable<VirtSpace> {
    friend MemoryManager;
    friend Region;

private:
    // TODO: Use a better data structure for regions.
    ustd::UniquePtr<Pml4> m_pml4;
    ustd::Vector<ustd::UniquePtr<Region>> m_regions;
    mutable SpinLock m_lock;

    VirtSpace();
    explicit VirtSpace(const ustd::Vector<ustd::UniquePtr<Region>> &regions);

    void map_4KiB(uintptr_t virt, uintptr_t phys, PageFlags flags);
    void map_2MiB(uintptr_t virt, uintptr_t phys, PageFlags flags);
    void map_1GiB(uintptr_t virt, uintptr_t phys, PageFlags flags);

public:
    VirtSpace(const VirtSpace &) = delete;
    VirtSpace(VirtSpace &&) = delete;
    ~VirtSpace() = default;

    VirtSpace &operator=(const VirtSpace &) = delete;
    VirtSpace &operator=(VirtSpace &&) = delete;

    auto begin() const { return m_regions.begin(); }
    auto end() const { return m_regions.end(); }

    Region &allocate_region(size_t size, RegionAccess access, ustd::Optional<uintptr_t> phys_base = {});
    Region &create_region(uintptr_t base, size_t size, RegionAccess access, ustd::Optional<uintptr_t> phys_base = {});
    ustd::SharedPtr<VirtSpace> clone() const;
};

} // namespace kernel
