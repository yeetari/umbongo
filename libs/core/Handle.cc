#include <core/Handle.hh>

#include <system/Syscall.hh>
#include <ustd/Try.hh>

namespace core {

Handle::~Handle() {
    if (m_handle != UB_NULL_HANDLE) {
        ASSUME(system::syscall(SYS_handle_close, m_handle));
    }
}

Handle &Handle::operator=(Handle &&other) {
    ustd::swap(m_handle, other.m_handle);
    return *this;
}

} // namespace core
