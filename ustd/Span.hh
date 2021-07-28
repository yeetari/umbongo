#pragma once

#include <ustd/Assert.hh>
#include <ustd/Traits.hh>
#include <ustd/Types.hh>

namespace ustd {

template <typename T>
class Span {
    T *m_data{nullptr};
    usize m_size{0};

public:
    constexpr Span() = default;
    constexpr Span(T *data, usize size) : m_data(data), m_size(size) {}

    template <typename U>
    constexpr Span<U> as() {
        return {static_cast<U *>(m_data), m_size};
    }
    template <typename U>
    constexpr Span<const U> as() const {
        return {static_cast<const U *>(m_data), m_size};
    }

    // Allow implicit conversion of `Span<T>` to `Span<void>`.
    constexpr operator Span<void>() { return {data(), size_bytes()}; }
    constexpr operator Span<const void>() const { return {data(), size_bytes()}; }

    constexpr T *begin() const { return m_data; }
    constexpr T *end() const { return m_data + m_size; }

    template <typename U = Conditional<IsSame<T, void>, char, T>>
    constexpr U &operator[](usize index) const requires(!IsSame<T, void>) {
        ASSERT(index < m_size);
        return begin()[index];
    }

    constexpr T *data() const { return m_data; }
    constexpr usize size() const { return m_size; }
    constexpr usize size_bytes() const { return m_size * sizeof(T); }
};

} // namespace ustd

using ustd::Span;
