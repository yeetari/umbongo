#pragma once

#include <kernel/HandleKind.hh>
#include <kernel/SysResult.hh>
#include <kernel/mem/PhysicalPage.hh>
#include <ustd/Shareable.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Vector.hh>

namespace kernel {

class VmObject : public ustd::Shareable<VmObject> {
    USTD_ALLOW_MAKE_SHARED;
    ustd::Vector<PhysicalPage> m_physical_pages;

    VmObject(ustd::Vector<PhysicalPage> &&physical_pages) : m_physical_pages(ustd::move(physical_pages)) {}

public:
    static constexpr auto k_handle_kind = HandleKind::VmObject;
    static ustd::SharedPtr<VmObject> create(size_t size);
    static ustd::SharedPtr<VmObject> create_physical(uintptr_t base, size_t size);

    size_t size() const;
    const ustd::Vector<PhysicalPage> &physical_pages() const { return m_physical_pages; }
};

} // namespace kernel
