#pragma once

#include <ustd/Numeric.hh>
#include <ustd/Traits.hh> // IWYU pragma: keep
#include <ustd/Types.hh>
#include <ustd/Utility.hh>

namespace ustd {

template <typename>
class Function;

template <typename R, typename... Args>
class Function<R(Args...)> { // NOLINT
    struct CallableBase {
        CallableBase() = default;
        CallableBase(const CallableBase &) = default;
        CallableBase(CallableBase &&) = default;
        virtual ~CallableBase() = default;

        CallableBase &operator=(const CallableBase &) = default;
        CallableBase &operator=(CallableBase &&) = default;

        virtual R call(Args...) = 0;
        virtual void move_to(void *) = 0;
    };

    template <typename F>
    class Callable final : public CallableBase {
        F m_callable;

    public:
        explicit Callable(F &&callable) : m_callable(move(callable)) {}

        R call(Args... args) override { return m_callable(forward<Args>(args)...); }
        void move_to(void *dst) override { new (dst) Callable{move(m_callable)}; }
    };

    static constexpr size_t k_inline_capacity = sizeof(void *) * 2;
    alignas(max(alignof(CallableBase), alignof(CallableBase *))) uint8_t m_storage[k_inline_capacity]; // NOLINT
    enum class State {
        Null,
        Inline,
        Outline,
    } m_state{State::Null};

    CallableBase *callable() const {
        switch (m_state) {
        case State::Null:
            return nullptr;
        case State::Inline:
            return reinterpret_cast<CallableBase *>(const_cast<Function *>(this)->m_storage);
        case State::Outline:
            return *reinterpret_cast<CallableBase **>(const_cast<Function *>(this)->m_storage);
        }
    }

    template <typename F>
    void set_callable(F &&callable) {
        using CallableType = Callable<F>;
        if constexpr (sizeof(CallableType) <= k_inline_capacity) {
            // NOLINTNEXTLINE
            new (m_storage) CallableType(forward<F>(callable));
            m_state = State::Inline;
        } else {
            // NOLINTNEXTLINE
            new (m_storage) CallableType *(new CallableType(forward<F>(callable)));
            m_state = State::Outline;
        }
    }

public:
    Function() = default; // NOLINT
    template <typename F>
    Function(F &&callable) requires(!is_same<F, Function<R(Args...)>>) { // NOLINT
        set_callable(forward<F>(callable));
    }
    Function(const Function &) = delete;
    Function(Function &&);
    ~Function();

    Function &operator=(const Function &) = delete;
    Function &operator=(Function &&);

    explicit operator bool() const { return m_state != State::Null; }
    R operator()(Args... args) const { return callable()->call(forward<Args>(args)...); }
};

template <typename R, typename... Args> // NOLINT
Function<R(Args...)>::Function(Function &&other) {
    *this = move(other);
}

template <typename R, typename... Args>
Function<R(Args...)>::~Function() {
    if (m_state == State::Inline) {
        callable()->~CallableBase();
    } else if (m_state == State::Outline) {
        delete callable();
    }
}

template <typename R, typename... Args>
Function<R(Args...)> &Function<R(Args...)>::operator=(Function &&other) {
    if (this != &other) {
        if (m_state == State::Inline) {
            callable()->~CallableBase();
        } else if (m_state == State::Outline) {
            delete callable();
        }
        m_state = other.m_state;
        if (m_state == State::Inline) {
            other.callable()->move_to(callable());
        } else if (m_state == State::Outline) {
            *reinterpret_cast<CallableBase **>(m_storage) = other.callable();
        }
        other.m_state = State::Null;
    }
    return *this;
}

} // namespace ustd
