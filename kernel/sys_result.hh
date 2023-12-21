#pragma once

#include <ustd/result.hh>
#include <ustd/types.hh>
#include <ustd/utility.hh>

namespace kernel {

enum class Error : ssize_t;

template <typename T = void>
using SysResult = ustd::Result<T, Error>;

class SyscallResult {
    size_t m_value;

public:
    SyscallResult(Error error) : m_value(static_cast<size_t>(error)) {}
    template <ustd::ConvertibleTo<size_t> T>
    SyscallResult(T value) : m_value(static_cast<size_t>(value)) {}
    template <typename T>
    SyscallResult(T *value) : m_value(reinterpret_cast<size_t>(value)) {}

    size_t value() const { return m_value; }
};

} // namespace kernel
