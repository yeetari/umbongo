#pragma once

#include <ustd/Assert.hh>
#include <ustd/Numeric.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>

// TODO: Inline stack contents.
// TODO: Small vectors - vectors with a small capacity and size so uint16 could be used instead of usize for example.
// TODO: Specialisation for trivially copyable types using memcpy, realloc, etc.

namespace ustd {

template <typename T>
class Vector {
    T *m_data{nullptr};
    usize m_capacity{0};
    usize m_size{0};

public:
    constexpr Vector() = default;
    explicit Vector(usize size);
    Vector(const Vector &) = delete;
    Vector(Vector &&other) noexcept
        : m_data(exchange(other.m_data, nullptr)), m_capacity(exchange(other.m_capacity, 0)),
          m_size(exchange(other.m_size, 0)) {}
    ~Vector();

    Vector &operator=(const Vector &) = delete;
    Vector &operator=(Vector &&) = delete;

    void ensure_capacity(usize capacity);
    void reallocate(usize capacity);
    void resize(usize size);

    template <typename... Args>
    T &emplace(Args &&...args);
    void push(const T &elem);
    void push(T &&elem);

    T &operator[](usize index);
    const T &operator[](usize index) const;

    T *begin() { return m_data; }
    T *end() { return m_data + m_size; }
    const T *begin() const { return m_data; }
    const T *end() const { return m_data + m_size; }

    bool empty() const { return m_size == 0; }
    T *data() const { return m_data; }
    usize capacity() const { return m_capacity; }
    usize size() const { return m_size; }
    usize size_in_bytes() const { return m_size * sizeof(T); }
};

template <typename T>
Vector<T>::Vector(usize size) {
    resize(size);
}

template <typename T>
Vector<T>::~Vector() {
    for (auto *elem = end(); elem != begin();) {
        (--elem)->~T();
    }
    delete m_data;
}

template <typename T>
void Vector<T>::ensure_capacity(usize capacity) {
    if (capacity > m_capacity) {
        reallocate(max(m_capacity * 2 + 1, capacity));
    }
}

template <typename T>
void Vector<T>::reallocate(usize capacity) {
    ASSERT(capacity >= m_size);
    T *new_data = reinterpret_cast<T *>(new uint8[capacity * sizeof(T)]);
    for (auto *data = new_data; auto &elem : *this) {
        new (data++) T(move(elem));
    }
    for (auto *elem = end(); elem != begin();) {
        (--elem)->~T();
    }
    delete m_data;
    m_data = new_data;
    m_capacity = capacity;
}

template <typename T>
void Vector<T>::resize(usize size) {
    ensure_capacity(m_size = size);
}

template <typename T>
template <typename... Args>
T &Vector<T>::emplace(Args &&...args) {
    ensure_capacity(m_size + 1);
    new (end()) T(forward<Args>(args)...);
    return (*this)[m_size++];
}

template <typename T>
void Vector<T>::push(const T &elem) {
    ensure_capacity(m_size + 1);
    new (end()) T(elem);
    m_size++;
}

template <typename T>
void Vector<T>::push(T &&elem) {
    ensure_capacity(m_size + 1);
    new (end()) T(move(elem));
    m_size++;
}

template <typename T>
T &Vector<T>::operator[](usize index) {
    ASSERT(index < m_size);
    return begin()[index];
}

template <typename T>
const T &Vector<T>::operator[](usize index) const {
    ASSERT(index < m_size);
    return begin()[index];
}

} // namespace ustd

using ustd::Vector;
