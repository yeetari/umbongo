#pragma once

#include <ustd/assert.hh>
#include <ustd/span.hh>
#include <ustd/types.hh>

namespace ustd {

template <typename T, size_t N>
struct Array {
    // NOLINTNEXTLINE
    alignas(T) T m_data[N];

    constexpr Span<T> span() { return {data(), size()}; }
    constexpr Span<const T> span() const { return {data(), size()}; }

    constexpr T *begin() { return data(); }
    constexpr T *end() { return data() + size(); }
    constexpr const T *begin() const { return data(); }
    constexpr const T *end() const { return data() + size(); }

    constexpr T &operator[](size_t index);
    constexpr const T &operator[](size_t index) const;

    constexpr T &first() { return begin()[0]; }
    constexpr const T &first() const { return begin()[0]; }
    constexpr T &last() { return end()[-1]; }
    constexpr const T &last() const { return end()[-1]; }

    constexpr bool empty() const { return N == 0; }
    constexpr T *data() { return static_cast<T *>(m_data); }
    constexpr const T *data() const { return static_cast<const T *>(m_data); }
    constexpr size_t size() const { return N; }
    constexpr size_t size_bytes() const { return N * sizeof(T); }
};

template <typename T, typename... Args>
Array(T, Args...) -> Array<T, sizeof...(Args) + 1>;

template <typename T, size_t N>
constexpr T &Array<T, N>::operator[](size_t index) {
    ASSERT(index < N);
    return begin()[index];
}

template <typename T, size_t N>
constexpr const T &Array<T, N>::operator[](size_t index) const {
    ASSERT(index < N);
    return begin()[index];
}

} // namespace ustd
