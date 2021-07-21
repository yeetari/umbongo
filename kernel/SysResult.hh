#pragma once

#include <kernel/SysError.hh>
#include <ustd/Assert.hh>
#include <ustd/Types.hh>

class SysResult {
    union {
        SysError m_error;
        usize m_value;
    };
    const bool m_is_error;

public:
    SysResult(SysError error) : m_error(error), m_is_error(true) {} // NOLINT
    template <typename T>
    SysResult(T value) : m_value(static_cast<usize>(value)), m_is_error(false) {} // NOLINT

    bool is_error() const { return m_is_error; }
    SysError error() const {
        ASSERT(m_is_error);
        return m_error;
    }
    usize value() const {
        ASSERT(!m_is_error);
        return m_value;
    }
};
