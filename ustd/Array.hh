#pragma once

#include <ustd/Types.hh>

template <typename T, uint32 N>
struct Array {
    // NOLINTNEXTLINE
    alignas(T) T m_data[N];

    // TODO: Assert index in bounds.
    T &operator[](uint32 index) { return m_data[index]; }
    const T &operator[](uint32 index) const { return m_data[index]; }

    T *data() { return static_cast<T *>(m_data); }
    const T *data() const { return static_cast<T *>(m_data); }

    constexpr uint32 size() const { return N; }
    constexpr uint32 size_in_bytes() const { return N * sizeof(T); }
};

template <typename T, typename... Args>
Array(T, Args...) -> Array<T, sizeof...(Args) + 1>;
