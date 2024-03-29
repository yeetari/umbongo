#pragma once

#include <ustd/assert.hh>
#include <ustd/numeric.hh>
#include <ustd/span.hh>
#include <ustd/types.hh>
#include <ustd/utility.hh>

// TODO: Inline stack contents.

namespace ustd {

template <typename T, typename SizeType = uint32_t>
class Vector {
    T *m_data{nullptr};
    SizeType m_capacity{0};
    SizeType m_size{0};

public:
    constexpr Vector() = default;
    template <typename... Args>
    explicit Vector(SizeType size, Args &&...args);
    Vector(const Vector &);
    Vector(Vector &&);
    ~Vector();

    Vector &operator=(const Vector &) = delete;
    Vector &operator=(Vector &&);

    void clear();
    void ensure_capacity(SizeType capacity);
    template <typename... Args>
    void ensure_size(SizeType size, Args &&...args);
    void reallocate(SizeType capacity);

    template <typename... Args>
    T &emplace(Args &&...args);
    template <typename... Args>
    T &emplace_at(SizeType index, Args &&...args);
    template <typename Container>
    void extend(const Container &container);
    void push(const T &elem);
    void push(T &&elem);
    conditional<is_trivially_copyable<T>, T, void> pop();
    void remove(SizeType index);
    T take(SizeType index);

    Span<T> span() { return {m_data, m_size}; }
    Span<const T> span() const { return {m_data, m_size}; }
    Span<T> take_all();

    T *begin() { return m_data; }
    T *end() { return m_data + m_size; }
    const T *begin() const { return m_data; }
    const T *end() const { return m_data + m_size; }

    T &operator[](SizeType index);
    const T &operator[](SizeType index) const;

    T &first() { return begin()[0]; }
    const T &first() const { return begin()[0]; }
    T &last() { return end()[-1]; }
    const T &last() const { return end()[-1]; }

    bool empty() const { return m_size == 0; }
    T *data() const { return m_data; }
    SizeType capacity() const { return m_capacity; }
    SizeType size() const { return m_size; }
    SizeType size_bytes() const { return m_size * sizeof(T); }
};

template <typename T>
using LargeVector = Vector<T, uint64_t>;

template <typename T, typename SizeType>
template <typename... Args>
Vector<T, SizeType>::Vector(SizeType size, Args &&...args) {
    ensure_size(size, forward<Args>(args)...);
}

template <typename T, typename SizeType>
Vector<T, SizeType>::Vector(const Vector &other) {
    ensure_capacity(other.size());
    m_size = other.size();
    if constexpr (!is_trivially_copyable<T>) {
        for (auto *data = m_data; const auto &elem : other) {
            new (data++) T(elem);
        }
    } else {
        __builtin_memcpy(m_data, other.data(), other.size_bytes());
    }
}

template <typename T, typename SizeType>
Vector<T, SizeType>::Vector(Vector &&other) {
    m_data = exchange(other.m_data, nullptr);
    m_capacity = exchange(other.m_capacity, SizeType(0));
    m_size = exchange(other.m_size, SizeType(0));
}

template <typename T, typename SizeType>
Vector<T, SizeType>::~Vector() {
    clear();
    delete[] reinterpret_cast<uint8_t *>(m_data);
}

template <typename T, typename SizeType>
Vector<T, SizeType> &Vector<T, SizeType>::operator=(Vector &&other) {
    if (this != &other) {
        clear();
        m_data = exchange(other.m_data, nullptr);
        m_capacity = exchange(other.m_capacity, 0u);
        m_size = exchange(other.m_size, 0u);
    }
    return *this;
}

template <typename T, typename SizeType>
void Vector<T, SizeType>::clear() {
    if constexpr (!is_trivially_destructible<T>) {
        for (auto *elem = end(); elem != begin();) {
            (--elem)->~T();
        }
    }
    m_size = 0;
}

template <typename T, typename SizeType>
void Vector<T, SizeType>::ensure_capacity(SizeType capacity) {
    if (capacity > m_capacity) {
        reallocate(max(m_capacity * 2 + 1, capacity));
    }
}

template <typename T, typename SizeType>
template <typename... Args>
void Vector<T, SizeType>::ensure_size(SizeType size, Args &&...args) {
    if (size <= m_size) {
        return;
    }
    ensure_capacity(size);
    if constexpr (!is_trivially_copyable<T> || sizeof...(Args) != 0) {
        for (SizeType i = m_size; i < size; i++) {
            new (begin() + i) T(forward<Args>(args)...);
        }
    } else {
        __builtin_memset(begin() + m_size, 0, size * sizeof(T) - m_size * sizeof(T));
    }
    m_size = size;
}

template <typename T, typename SizeType>
void Vector<T, SizeType>::reallocate(SizeType capacity) {
    ASSERT(capacity >= m_size);
    T *new_data = reinterpret_cast<T *>(new uint8_t[capacity * sizeof(T)]);
    if constexpr (!is_trivially_copyable<T>) {
        for (auto *data = new_data; auto &elem : *this) {
            new (data++) T(move(elem));
        }
        for (auto *elem = end(); elem != begin();) {
            (--elem)->~T();
        }
    } else {
        __builtin_memcpy(new_data, m_data, size_bytes());
    }
    delete[] reinterpret_cast<uint8_t *>(m_data);
    m_data = new_data;
    m_capacity = capacity;
}

template <typename T, typename SizeType>
template <typename... Args>
T &Vector<T, SizeType>::emplace(Args &&...args) {
    ensure_capacity(m_size + 1);
    new (end()) T(forward<Args>(args)...);
    return (*this)[m_size++];
}

template <typename T, typename SizeType>
template <typename... Args>
T &Vector<T, SizeType>::emplace_at(SizeType index, Args &&...args) {
    if (index == m_size) {
        return emplace(forward<Args>(args)...);
    }
    ASSERT(index < m_size);
    ensure_capacity(m_size + 1);
    auto *elem = &m_data[index];
    new (end()) T(move(end()[-1]));

    // TODO: Create a generic ustd::move_backward algorithm function also with IsTriviallyCopyable specialisation.
    auto *src = end() - 1;
    auto *dst = end();
    while (src != elem) {
        *--dst = ustd::move(*--src);
    }

    m_size++;
    return *new (elem) T(forward<Args>(args)...);
}

template <typename T, typename SizeType>
template <typename Container>
void Vector<T, SizeType>::extend(const Container &container) {
    if (container.empty()) {
        return;
    }
    ensure_capacity(m_size + SizeType(container.size()));
    if constexpr (is_trivially_copyable<T>) {
        __builtin_memcpy(end(), container.data(), container.size_bytes());
        m_size += SizeType(container.size());
    } else {
        for (const auto &elem : container) {
            push(elem);
        }
    }
}

template <typename T, typename SizeType>
void Vector<T, SizeType>::push(const T &elem) {
    ensure_capacity(m_size + 1);
    if constexpr (is_trivially_copyable<T>) {
        __builtin_memcpy(end(), &elem, sizeof(T));
    } else {
        new (end()) T(elem);
    }
    m_size++;
}

template <typename T, typename SizeType>
void Vector<T, SizeType>::push(T &&elem) {
    ensure_capacity(m_size + 1);
    new (end()) T(move(elem));
    m_size++;
}

template <typename T, typename SizeType>
conditional<is_trivially_copyable<T>, T, void> Vector<T, SizeType>::pop() {
    ASSERT(!empty());
    m_size--;
    auto *elem = end();
    if constexpr (is_trivially_copyable<T>) {
        return *elem;
    }
    if constexpr (!is_trivially_destructible<T>) {
        elem->~T();
    }
}

template <typename T, typename SizeType>
void Vector<T, SizeType>::remove(SizeType index) {
    ASSERT(index < m_size);
    if constexpr (!is_trivially_destructible<T>) {
        begin()[index].~T();
    }
    m_size--;
    if constexpr (is_trivially_copyable<T>) {
        __builtin_memcpy(begin() + index, begin() + index + 1, (m_size - index) * sizeof(T));
        return;
    }
    for (SizeType i = index; i < m_size; i++) {
        new (begin() + i) T(move(begin()[i + 1]));
        begin()[i + 1].~T();
    }
}

template <typename T, typename SizeType>
T Vector<T, SizeType>::take(SizeType index) {
    ASSERT(index < m_size);
    auto elem = move(begin()[index]);
    remove(index);
    return elem;
}

template <typename T, typename SizeType>
Span<T> Vector<T, SizeType>::take_all() {
    m_capacity = 0u;
    return {exchange(m_data, nullptr), exchange(m_size, 0u)};
}

template <typename T, typename SizeType>
T &Vector<T, SizeType>::operator[](SizeType index) {
    ASSERT(index < m_size);
    return begin()[index];
}

template <typename T, typename SizeType>
const T &Vector<T, SizeType>::operator[](SizeType index) const {
    ASSERT(index < m_size);
    return begin()[index];
}

} // namespace ustd
