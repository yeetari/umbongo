#pragma once

#include <ustd/Types.hh>

namespace ustd {

template <typename T>
class Span {
    T *const m_data;
    const usize m_size;

public:
    constexpr Span(T *data, usize size) : m_data(data), m_size(size) {}

    // Allow implicit conversion of Span<T> to Span<void>.
    constexpr operator Span<void>() { return {m_data, m_size}; }
    constexpr operator Span<const void>() const { return {m_data, m_size}; }

    constexpr T *begin() const { return m_data; }
    constexpr T *end() const { return m_data + m_size; }

    constexpr T *data() const { return m_data; }
    constexpr usize size() const { return m_size; }
    constexpr usize size_in_bytes() const { return m_size * sizeof(T); }
};

} // namespace ustd

using ustd::Span;