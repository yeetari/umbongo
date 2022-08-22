#pragma once

#include <ustd/Assert.hh>
#include <ustd/Utility.hh>

namespace ustd {

template <typename T>
class UniquePtr {
    T *m_ptr{nullptr};

public:
    constexpr UniquePtr() = default;
    explicit UniquePtr(T *ptr) : m_ptr(ptr) {}
    UniquePtr(const UniquePtr &) = delete;
    UniquePtr(UniquePtr &&other) : m_ptr(other.disown()) {}
    template <typename U>
    UniquePtr(UniquePtr<U> &&other) : m_ptr(other.disown()) {}
    ~UniquePtr() { delete m_ptr; }

    UniquePtr &operator=(const UniquePtr &) = delete;
    UniquePtr &operator=(UniquePtr &&);

    void clear();
    T *disown();
    template <typename U = T, typename... Args>
    U &emplace(Args &&...args);

    explicit operator bool() const { return m_ptr != nullptr; }
    bool has_value() const { return m_ptr != nullptr; }

    T &operator*() const;
    T *operator->() const;
    T *ptr() const { return m_ptr; }
};

template <typename T>
UniquePtr<T> &UniquePtr<T>::operator=(UniquePtr &&other) {
    UniquePtr moved(move(other));
    swap(m_ptr, moved.m_ptr);
    ASSERT(m_ptr != nullptr);
    return *this;
}

template <typename T>
void UniquePtr<T>::clear() {
    delete exchange(m_ptr, nullptr);
}

template <typename T>
T *UniquePtr<T>::disown() {
    return exchange(m_ptr, nullptr);
}

template <typename T>
template <typename U, typename... Args>
U &UniquePtr<T>::emplace(Args &&...args) {
    clear();
    m_ptr = new U(forward<Args>(args)...);
    return static_cast<U &>(operator*());
}

template <typename T>
T &UniquePtr<T>::operator*() const {
    ASSERT(m_ptr != nullptr);
    return *m_ptr;
}

template <typename T>
T *UniquePtr<T>::operator->() const {
    ASSERT(m_ptr != nullptr);
    return m_ptr;
}

template <typename T, typename... Args>
UniquePtr<T> make_unique(Args &&...args) {
    return UniquePtr<T>(new T(forward<Args>(args)...));
}

} // namespace ustd
