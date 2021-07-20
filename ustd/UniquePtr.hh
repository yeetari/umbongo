#pragma once

#include <ustd/Utility.hh>

namespace ustd {

template <typename T>
class UniquePtr {
    T *m_obj{nullptr};

public:
    constexpr UniquePtr() = default;
    explicit UniquePtr(T *obj) : m_obj(obj) {}
    UniquePtr(const UniquePtr &) = delete;
    UniquePtr(UniquePtr &&other) noexcept : m_obj(other.disown()) {}
    template <typename U>
    UniquePtr(UniquePtr<U> &&other) noexcept : m_obj(other.disown()) {}
    ~UniquePtr() { delete m_obj; }

    UniquePtr &operator=(const UniquePtr &) = delete;
    UniquePtr &operator=(UniquePtr &&other) noexcept;

    T &operator*() const {
        ASSERT(m_obj != nullptr);
        return *m_obj;
    }
    T *operator->() const {
        ASSERT(m_obj != nullptr);
        return m_obj;
    }

    T *disown() { return exchange(m_obj, nullptr); }
    T *obj() const { return m_obj; }
};

template <typename T>
UniquePtr<T> &UniquePtr<T>::operator=(UniquePtr &&other) noexcept {
    UniquePtr ptr(move(other));
    swap(m_obj, ptr.m_obj);
    ASSERT(m_obj != nullptr);
    return *this;
}

template <typename T, typename... Args>
UniquePtr<T> make_unique(Args &&...args) {
    return UniquePtr<T>(new T(forward<Args>(args)...));
}

} // namespace ustd

using ustd::UniquePtr;