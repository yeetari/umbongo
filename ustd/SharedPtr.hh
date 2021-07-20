#pragma once

#include <ustd/Shareable.hh>
#include <ustd/Utility.hh>

namespace ustd {

template <typename T>
class SharedPtr {
    template <typename U>
    friend class SharedPtr;

private:
    T *m_obj{nullptr};

public:
    constexpr SharedPtr() = default;
    explicit SharedPtr(T *obj);
    SharedPtr(const SharedPtr &other) : SharedPtr(other.m_obj) {}
    template <typename U>
    SharedPtr(const SharedPtr<U> &other) : SharedPtr(other.m_obj) {}
    SharedPtr(SharedPtr &&other) noexcept : m_obj(exchange(other.m_obj, nullptr)) {}
    template <typename U>
    SharedPtr(SharedPtr<U> &&other) noexcept : m_obj(exchange(other.m_obj, nullptr)) {}
    ~SharedPtr();

    SharedPtr &operator=(const SharedPtr &) = delete;
    SharedPtr &operator=(SharedPtr &&) = delete;

    T &operator*() const {
        ASSERT(m_obj != nullptr);
        return *m_obj;
    }
    T *operator->() const {
        ASSERT(m_obj != nullptr);
        return m_obj;
    }
};

template <typename T>
SharedPtr<T>::SharedPtr(T *obj) : m_obj(obj) {
    if (obj) {
        obj->add_ref();
    }
}

template <typename T>
SharedPtr<T>::~SharedPtr() {
    if (m_obj != nullptr) {
        m_obj->sub_ref();
    }
}

template <typename T, typename... Args>
SharedPtr<T> make_shared(Args &&...args) {
    return SharedPtr<T>(new T(forward<Args>(args)...));
}

} // namespace ustd

using ustd::SharedPtr;
