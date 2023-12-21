#pragma once

#include <ustd/Assert.hh>
#include <ustd/Shareable.hh> // IWYU pragma: keep
#include <ustd/Utility.hh>

namespace ustd {

template <typename T>
class SharedPtr {
    template <typename U>
    friend class SharedPtr;

private:
    T *m_ptr{nullptr};

public:
    constexpr SharedPtr() = default;
    explicit SharedPtr(T *ptr);
    SharedPtr(const SharedPtr &other) : SharedPtr(other.m_ptr) {}
    template <typename U>
    SharedPtr(const SharedPtr<U> &other) : SharedPtr(other.m_ptr) {}
    SharedPtr(SharedPtr &&other) : m_ptr(other.disown()) {}
    template <typename U>
    SharedPtr(SharedPtr<U> &&other) : m_ptr(other.disown()) {}
    ~SharedPtr();

    SharedPtr &operator=(const SharedPtr &) = delete;
    SharedPtr &operator=(SharedPtr &&);

    void clear();
    T *disown();

    explicit operator bool() const { return m_ptr != nullptr; }
    bool has_value() const { return m_ptr != nullptr; }

    T &operator*() const;
    T *operator->() const;
    T *ptr() const { return m_ptr; }
};

template <typename T>
SharedPtr<T>::SharedPtr(T *ptr) : m_ptr(ptr) {
    if (ptr != nullptr) {
        ptr->add_ref();
    }
}

template <typename T>
SharedPtr<T>::~SharedPtr() {
    if (m_ptr != nullptr) {
        m_ptr->sub_ref();
    }
}

template <typename T>
SharedPtr<T> &SharedPtr<T>::operator=(SharedPtr &&other) {
    SharedPtr moved(move(other));
    swap(m_ptr, moved.m_ptr);
    ASSERT(m_ptr != nullptr);
    return *this;
}

template <typename T>
void SharedPtr<T>::clear() {
    if (m_ptr != nullptr) {
        m_ptr->sub_ref();
    }
    m_ptr = nullptr;
}

template <typename T>
T *SharedPtr<T>::disown() {
    return exchange(m_ptr, nullptr);
}

template <typename T>
T &SharedPtr<T>::operator*() const {
    ASSERT(m_ptr != nullptr);
    return *m_ptr;
}

template <typename T>
T *SharedPtr<T>::operator->() const {
    ASSERT(m_ptr != nullptr);
    return m_ptr;
}

template <typename T>
SharedPtr<T> adopt_shared(T *ptr) {
    return SharedPtr<T>(ptr);
}

template <typename T, typename... Args>
SharedPtr<T> make_shared(Args &&...args) {
    return SharedPtr<T>(new T(forward<Args>(args)...));
}

} // namespace ustd

#define USTD_ALLOW_MAKE_SHARED                                                                                         \
    template <typename T, typename... Args>                                                                            \
    friend ustd::SharedPtr<T> ustd::make_shared(Args &&...)
