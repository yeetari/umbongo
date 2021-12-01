#pragma once

#include <ustd/Assert.hh>
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
    SharedPtr(SharedPtr &&other) noexcept : m_obj(other.disown()) {}
    template <typename U>
    SharedPtr(SharedPtr<U> &&other) noexcept : m_obj(other.disown()) {}
    ~SharedPtr();

    SharedPtr &operator=(const SharedPtr &) = delete;
    SharedPtr &operator=(SharedPtr &&) noexcept;

    explicit operator bool() const noexcept { return m_obj != nullptr; }
    bool has_value() const noexcept { return m_obj != nullptr; }

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
SharedPtr<T>::SharedPtr(T *obj) : m_obj(obj) {
    if (obj != nullptr) {
        obj->add_ref();
    }
}

template <typename T>
SharedPtr<T>::~SharedPtr() {
    if (m_obj != nullptr) {
        m_obj->sub_ref();
    }
}

template <typename T>
SharedPtr<T> &SharedPtr<T>::operator=(SharedPtr &&other) noexcept {
    SharedPtr ptr(move(other));
    swap(m_obj, ptr.m_obj);
    ASSERT(m_obj != nullptr);
    return *this;
}

template <typename T, typename... Args>
SharedPtr<T> make_shared(Args &&...args) {
    return SharedPtr<T>(new T(forward<Args>(args)...));
}

} // namespace ustd
