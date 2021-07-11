#pragma once

#include <ustd/Assert.hh>
#include <ustd/Span.hh>
#include <ustd/Types.hh>

namespace ustd {

template <typename T, usize N>
struct Array {
    // NOLINTNEXTLINE
    alignas(T) T m_data[N];

    // Allow implicit conversion from `Array<T>` to `Span<T>`, or from `const Array<T>` to `Span<const T>`.
    constexpr operator Span<T>() { return {data(), size()}; }
    constexpr operator Span<const T>() const { return {data(), size()}; }

    constexpr T *begin() { return data(); }
    constexpr T *end() { return data() + size(); }
    constexpr const T *begin() const { return data(); }
    constexpr const T *end() const { return data() + size(); }

    constexpr T &operator[](usize index) {
        ASSERT(index < N);
        return begin()[index];
    }
    constexpr const T &operator[](usize index) const {
        ASSERT(index < N);
        return begin()[index];
    }

    constexpr T *data() { return static_cast<T *>(m_data); }
    constexpr const T *data() const { return static_cast<const T *>(m_data); }
    constexpr usize size() const { return N; }
    constexpr usize size_bytes() const { return N * sizeof(T); }
};

template <typename T, typename... Args>
Array(T, Args...) -> Array<T, sizeof...(Args) + 1>;

} // namespace ustd

using ustd::Array;
