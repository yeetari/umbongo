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

class VirtSpace : public Shareable<VirtSpace> {
    friend MemoryManager;
    friend Region;

private:
    // TODO: Use a better data structure for regions.
    UniquePtr<Pml4> m_pml4;
    Vector<UniquePtr<Region>> m_regions;
    mutable SpinLock m_lock;

    VirtSpace();
    explicit VirtSpace(const Vector<UniquePtr<Region>> &regions);

    void map_4KiB(uintptr virt, uintptr phys, PageFlags flags = static_cast<PageFlags>(0));
    void map_1GiB(uintptr virt, uintptr phys, PageFlags flags = static_cast<PageFlags>(0));

public:
    VirtSpace(const VirtSpace &) = delete;
    VirtSpace(VirtSpace &&) = delete;
    ~VirtSpace() = default;

    VirtSpace &operator=(const VirtSpace &) = delete;
    VirtSpace &operator=(VirtSpace &&) = delete;

    Region &allocate_region(usize size, RegionAccess access, Optional<uintptr> phys_base = {});
    Region &create_region(uintptr base, usize size, RegionAccess access, Optional<uintptr> phys_base = {});
    SharedPtr<VirtSpace> clone() const;
};
