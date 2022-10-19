#pragma once

#include <system/Types.h>
#include <ustd/Utility.hh>

namespace core {

class Handle {
    ub_handle_t m_handle;

    friend Handle wrap_handle(ub_handle_t);
    Handle(ub_handle_t handle) : m_handle(handle) {}

public:
    Handle(const Handle &) = delete;
    Handle(Handle &&other) : m_handle(ustd::exchange(other.m_handle, UB_NULL_HANDLE)) {}
    ~Handle();

    Handle &operator=(const Handle &) = delete;
    Handle &operator=(Handle &&);

    ub_handle_t operator*() const { return m_handle; }
};

inline Handle wrap_handle(ub_handle_t handle) {
    return handle;
}

} // namespace core
