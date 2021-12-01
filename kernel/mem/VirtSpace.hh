#pragma once

#include <kernel/SpinLock.hh>
#include <kernel/cpu/Paging.hh>
#include <kernel/mem/Region.hh>
#include <ustd/Array.hh>
#include <ustd/Optional.hh>
#include <ustd/Shareable.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>
#include <ustd/Vector.hh>

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

    void map_4KiB(uintptr virt, uintptr phys, PageFlags flags);
    void map_2MiB(uintptr virt, uintptr phys, PageFlags flags);
    void map_1GiB(uintptr virt, uintptr phys, PageFlags flags);

public:
    VirtSpace(const VirtSpace &) = delete;
    VirtSpace(VirtSpace &&) = delete;
    ~VirtSpace() = default;

    VirtSpace &operator=(const VirtSpace &) = delete;
    VirtSpace &operator=(VirtSpace &&) = delete;

    Region &allocate_region(usize size, RegionAccess access, ustd::Optional<uintptr> phys_base = {});
    Region &create_region(uintptr base, usize size, RegionAccess access, ustd::Optional<uintptr> phys_base = {});
    ustd::SharedPtr<VirtSpace> clone() const;
};
