#pragma once

#include <ustd/Assert.hh>
#include <ustd/Memory.hh>
#include <ustd/Numeric.hh>
#include <ustd/Span.hh>
#include <ustd/Traits.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>

// TODO: Inline stack contents.

namespace ustd {

template <typename T, typename SizeType = uint32>
class Vector {
    T *m_data{nullptr};
    SizeType m_capacity{0};
    SizeType m_size{0};

public:
    constexpr Vector() = default;
    template <typename... Args>
    explicit Vector(SizeType size, Args &&...args);
    Vector(const Vector &other);
    Vector(Vector &&other) noexcept
        : m_data(exchange(other.m_data, nullptr)), m_capacity(exchange(other.m_capacity, 0u)),
          m_size(exchange(other.m_size, 0u)) {}
    ~Vector();

    Vector &operator=(const Vector &) = delete;
    Vector &operator=(Vector &&) noexcept;

    void clear();
    void ensure_capacity(SizeType capacity);
    void reallocate(SizeType capacity);
    template <typename... Args>
    void grow(SizeType size, Args &&...args);

    template <typename... Args>
    T &emplace(Args &&...args);
    template <typename... Args>
    T &emplace_at(SizeType index, Args &&...args);
    void push(const T &elem);
    void push(T &&elem);
    Conditional<IsTriviallyCopyable<T>, T, void> pop();
    void remove(SizeType index);
    T take(SizeType index);

    Span<T> span() { return {m_data, m_size}; }
    Span<const T> span() const { return {m_data, m_size}; }

    T *begin() { return m_data; }
    T *end() { return m_data + m_size; }
    const T *begin() const { return m_data; }
    const T *end() const { return m_data + m_size; }

    T &operator[](SizeType index);
    const T &operator[](SizeType index) const;

    bool empty() const { return m_size == 0; }
    T *data() const { return m_data; }
    SizeType capacity() const { return m_capacity; }
    SizeType size() const { return m_size; }
    SizeType size_bytes() const { return m_size * sizeof(T); }
};

template <typename T>
using LargeVector = Vector<T, usize>;

template <typename T, typename SizeType>
template <typename... Args>
Vector<T, SizeType>::Vector(SizeType size, Args &&...args) {
    grow(size, forward<Args>(args)...);
}

template <typename T, typename SizeType>
Vector<T, SizeType>::Vector(const Vector &other) {
    ensure_capacity(other.size());
    m_size = other.size();
    if constexpr (!IsTriviallyCopyable<T>) {
        for (auto *data = m_data; const auto &elem : other) {
            new (data++) T(elem);
        }
    } else {
        memcpy(m_data, other.data(), other.size_bytes());
    }
}

template <typename T, typename SizeType>
Vector<T, SizeType>::~Vector() {
    if constexpr (!IsTriviallyDestructible<T>) {
        for (auto *elem = end(); elem != begin();) {
            (--elem)->~T();
        }
    }
    delete reinterpret_cast<uint8 *>(m_data);
}

template <typename T, typename SizeType>
Vector<T, SizeType> &Vector<T, SizeType>::operator=(Vector &&other) noexcept {
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
    if constexpr (!IsTriviallyDestructible<T>) {
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
void Vector<T, SizeType>::reallocate(SizeType capacity) {
    ASSERT(capacity >= m_size);
    T *new_data = reinterpret_cast<T *>(new uint8[capacity * sizeof(T)]);
    if constexpr (!IsTriviallyCopyable<T>) {
        for (auto *data = new_data; auto &elem : *this) {
            new (data++) T(move(elem));
        }
        for (auto *elem = end(); elem != begin();) {
            (--elem)->~T();
        }
    } else {
        memcpy(new_data, m_data, size_bytes());
    }
    delete reinterpret_cast<uint8 *>(m_data);
    m_data = new_data;
    m_capacity = capacity;
}

template <typename T, typename SizeType>
template <typename... Args>
void Vector<T, SizeType>::grow(SizeType size, Args &&...args) {
    if (size <= m_size) {
        return;
    }
    ensure_capacity(size);
    if constexpr (!IsTriviallyCopyable<T> || sizeof...(Args) != 0) {
        for (SizeType i = m_size; i < size; i++) {
            new (begin() + i) T(forward<Args>(args)...);
        }
    } else {
        memset(begin() + m_size, 0, size * sizeof(T) - m_size * sizeof(T));
    }
    m_size = size;
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
void Vector<T, SizeType>::push(const T &elem) {
    ensure_capacity(m_size + 1);
    if constexpr (IsTriviallyCopyable<T>) {
        memcpy(end(), &elem, sizeof(T));
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
Conditional<IsTriviallyCopyable<T>, T, void> Vector<T, SizeType>::pop() {
    ASSERT(!empty());
    m_size--;
    auto *elem = end();
    if constexpr (IsTriviallyCopyable<T>) {
        return *elem;
    }
    if constexpr (!IsTriviallyDestructible<T>) {
        elem->~T();
    }
}

template <typename T, typename SizeType>
void Vector<T, SizeType>::remove(SizeType index) {
    ASSERT(index < m_size);
    if constexpr (!IsTriviallyDestructible<T>) {
        begin()[index].~T();
    }
    m_size--;
    if constexpr (IsTriviallyCopyable<T>) {
        memcpy(begin() + index, begin() + index + 1, (m_size - index) * sizeof(T));
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

using ustd::LargeVector;
using ustd::Vector;
