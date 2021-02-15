#pragma once

#include <ustd/Types.hh>

template <typename T, usize N>
struct Array {
    // NOLINTNEXTLINE
    alignas(T) T m_data[N];

    // TODO: Assert index in bounds.
    constexpr T &operator[](usize index) { return m_data[index]; }
    constexpr const T &operator[](usize index) const { return m_data[index]; }

    constexpr T *data() { return static_cast<T *>(m_data); }
    constexpr const T *data() const { return static_cast<const T *>(m_data); }

    constexpr usize size() const { return N; }
    constexpr usize size_in_bytes() const { return N * sizeof(T); }
};

template <typename T, typename... Args>
Array(T, Args...) -> Array<T, sizeof...(Args) + 1>;
