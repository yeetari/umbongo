#pragma once

#include <ustd/Result.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>

namespace kernel {

enum class SysError : ssize_t;

template <typename T = void>
using SysResult = ustd::Result<T, SysError>;

class SyscallResult {
    size_t m_value;

public:
    SyscallResult(SysError error) : m_value(static_cast<size_t>(error)) {}
    template <ustd::ConvertibleTo<size_t> T>
    SyscallResult(T value) : m_value(static_cast<size_t>(value)) {}
    template <typename T>
    SyscallResult(T *value) : m_value(reinterpret_cast<size_t>(value)) {}
    SyscallResult(SysResult<> result) : m_value(0) {
        if (result.is_error()) {
            m_value = static_cast<size_t>(result.error());
        }
    }

    size_t value() const { return m_value; }
};

} // namespace kernel
