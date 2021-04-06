#pragma once

#include <ustd/Assert.hh>
#include <ustd/Span.hh>
#include <ustd/Types.hh>

namespace ustd {

template <typename T, usize N>
struct Array {
    // NOLINTNEXTLINE
    alignas(T) T m_data[N];

    constexpr T &operator[](usize index) {
        ASSERT(index < N);
        return m_data[index];
    }
    constexpr const T &operator[](usize index) const {
        ASSERT(index < N);
        return m_data[index];
    }

    constexpr operator Span<T>() { return {data(), size()}; }
    constexpr operator Span<const T>() const { return {data(), size()}; }

    constexpr T *data() { return static_cast<T *>(m_data); }
    constexpr const T *data() const { return static_cast<const T *>(m_data); }

    constexpr usize size() const { return N; }
    constexpr usize size_in_bytes() const { return N * sizeof(T); }
};

template <typename T, typename... Args>
Array(T, Args...) -> Array<T, sizeof...(Args) + 1>;

} // namespace ustd

using ustd::Array;
