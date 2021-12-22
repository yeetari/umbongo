#pragma once

#include <kernel/SysError.hh>
#include <ustd/Assert.hh>
#include <ustd/Concepts.hh>
#include <ustd/Result.hh>
#include <ustd/Types.hh>

namespace kernel {

template <typename T = void>
using SysResult = ustd::Result<T, SysError>;

class SyscallResult {
    usize m_value;

public:
    SyscallResult(SysError error) : m_value(static_cast<usize>(error)) {}
    template <ustd::ConvertibleTo<usize> T>
    SyscallResult(T value) : m_value(static_cast<usize>(value)) {}
    template <typename T>
    SyscallResult(T *value) : m_value(reinterpret_cast<usize>(value)) {}
    SyscallResult(SysResult<> result) : m_value(0) {
        if (result.is_error()) {
            m_value = static_cast<usize>(result.error());
        }
    }

    usize value() const { return m_value; }
};

} // namespace kernel
