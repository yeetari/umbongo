#pragma once

#include <ustd/Types.hh>

namespace kernel {

class VirtualRange {
    const uintptr_t m_base;
    const size_t m_size;

public:
    VirtualRange(uintptr_t base, size_t size) : m_base(base), m_size(size) {}

    uintptr_t end() const { return m_base + m_size; }
    uintptr_t base() const { return m_base; }
    size_t size() const { return m_size; }
};

} // namespace kernel
