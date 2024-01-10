#pragma once

#include <kernel/mem/physical_page.hh>
#include <ustd/shareable.hh>
#include <ustd/shared_ptr.hh>
#include <ustd/vector.hh>

namespace kernel {

class VmObject : public ustd::Shareable<VmObject> {
    USTD_ALLOW_MAKE_SHARED;

private:
    ustd::Vector<PhysicalPage> m_physical_pages;
    size_t m_size;

    VmObject(ustd::Vector<PhysicalPage> &&physical_pages, size_t size)
        : m_physical_pages(ustd::move(physical_pages)), m_size(size) {}

public:
    static ustd::SharedPtr<VmObject> create(size_t size);
    static ustd::SharedPtr<VmObject> create_physical(uintptr_t base, size_t size);

    const ustd::Vector<PhysicalPage> &physical_pages() const { return m_physical_pages; }
    size_t size() const { return m_size; }
};

} // namespace kernel
