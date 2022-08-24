#pragma once

#include <ustd/Assert.hh>
#include <ustd/Traits.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>

namespace ustd {

template <typename T>
class Span {
    T *m_data{nullptr};
    size_t m_size{0};

    static constexpr bool is_void = is_same<remove_cv<T>, void>;
    using no_void_t = conditional<is_void, char, T>;

public:
    constexpr Span() = default;
    constexpr Span(T *data, size_t size) : m_data(data), m_size(size) {}

    template <typename U>
    constexpr Span<U> as() const;

    // Allow implicit conversion of `Span<T>` to `Span<void>`.
    constexpr operator Span<void>() const requires(!is_const<T>) { return {data(), size_bytes()}; }
    constexpr operator Span<const void>() const { return {data(), size_bytes()}; }

    constexpr T *begin() const { return m_data; }
    constexpr T *end() const { return m_data + m_size; }

    template <typename U = no_void_t>
    constexpr U &operator[](size_t index) const requires(!is_void);
    constexpr bool empty() const { return m_size == 0; }
    constexpr T *data() const { return m_data; }
    constexpr size_t size() const { return m_size; }
    constexpr size_t size_bytes() const { return m_size * sizeof(T); }
};

template <typename T>
template <typename U>
constexpr Span<U> Span<T>::as() const {
    return {static_cast<U *>(m_data), m_size};
}

template <typename T>
template <typename U>
constexpr U &Span<T>::operator[](size_t index) const requires(!is_void) {
    return begin()[index];
}

} // namespace ustd
