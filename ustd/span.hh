#pragma once

#include <ustd/types.hh>
#include <ustd/utility.hh>

namespace ustd {

template <typename T>
class Span {
    T *m_data{nullptr};
    size_t m_size{0};

    template <typename U>
    using const_t = conditional<is_const<T>, const U, U>;

    static constexpr bool is_void = is_same<remove_cv<T>, void>;
    using no_void_t = conditional<is_void, char, T>;

public:
    constexpr Span() = default;
    constexpr Span(T *data, size_t size) : m_data(data), m_size(size) {}

    template <typename U>
    constexpr Span<U> as() const;
    constexpr const_t<uint8_t> *byte_offset(size_t offset) const;

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
constexpr typename Span<T>::template const_t<uint8_t> *Span<T>::byte_offset(size_t offset) const {
    return as<const_t<uint8_t>>().data() + offset;
}

template <typename T>
template <typename U>
constexpr U &Span<T>::operator[](size_t index) const requires(!is_void) {
    return begin()[index];
}

template <typename T>
constexpr Span<T> make_span(T *data, size_t size) {
    return {data, size};
}

} // namespace ustd
