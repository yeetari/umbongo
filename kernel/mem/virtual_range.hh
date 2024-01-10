#pragma once

#include <ustd/types.hh>

namespace kernel {

class VirtualRange {
    uintptr_t m_base;
    size_t m_size;

public:
    VirtualRange(uintptr_t base, size_t size) : m_base(base), m_size(size) {}

    uintptr_t end() const { return m_base + m_size; }
    uintptr_t base() const { return m_base; }
    size_t size() const { return m_size; }
};

} // namespace kernel
