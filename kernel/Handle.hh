#pragma once

#include <kernel/HandleKind.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/UniquePtr.hh>

namespace kernel {

class HandleBase {
    const HandleKind m_kind;

protected:
    explicit HandleBase(HandleKind kind) : m_kind(kind) {}

public:
    HandleBase(const HandleBase &) = delete;
    HandleBase(HandleBase &&) = delete;
    virtual ~HandleBase() = default;

    HandleBase &operator=(const HandleBase &) = delete;
    HandleBase &operator=(HandleBase &&) = delete;

    virtual ustd::UniquePtr<HandleBase> copy() const = 0;

    HandleKind kind() const { return m_kind; }
};

template <typename T>
class Handle final : public HandleBase {
    ustd::SharedPtr<T> m_object;

public:
    explicit Handle(ustd::SharedPtr<T> object) : HandleBase(T::k_handle_kind), m_object(ustd::move(object)) {}

    ustd::UniquePtr<HandleBase> copy() const override;
    ustd::SharedPtr<T> object() const { return m_object; }
};

template <typename T>
ustd::UniquePtr<HandleBase> Handle<T>::copy() const {
    return ustd::make_unique<Handle<T>>(m_object);
}

} // namespace kernel
