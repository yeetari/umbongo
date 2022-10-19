#pragma once

#include <ustd/Types.hh>

namespace kernel {

template <typename T>
class UserPtr {
    uintptr_t m_ptr;

    UserPtr(uintptr_t ptr) : m_ptr(ptr) {}

public:
    static UserPtr from_raw(T *ptr) { return reinterpret_cast<uintptr_t>(ptr); }

    explicit operator bool() const { return m_ptr != 0; }
    UserPtr offseted(size_t offset) const { return m_ptr + offset * sizeof(T); }
    uintptr_t ptr_unsafe() const { return m_ptr; }
};

} // namespace kernel
