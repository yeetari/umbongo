#pragma once

#include <ustd/Array.hh>
#include <ustd/Concepts.hh>
#include <ustd/Types.hh>

namespace ustd {

enum class MemoryOrder {
    Relaxed = __ATOMIC_RELAXED,
    Consume = __ATOMIC_CONSUME,
    Acquire = __ATOMIC_ACQUIRE,
    Release = __ATOMIC_RELEASE,
    AcqRel = __ATOMIC_ACQ_REL,
    SeqCst = __ATOMIC_SEQ_CST,
};

template <typename T>
class Atomic {
    T m_value{0};

public:
    Atomic() = default;
    Atomic(T value) : m_value(value) {}
    Atomic(const Atomic &) = delete;
    Atomic(Atomic &&) = delete;
    ~Atomic() = default;

    Atomic &operator=(const Atomic &) = delete;
    Atomic &operator=(Atomic &&) = delete;

    T load(MemoryOrder order) const volatile;
    void store(T value, MemoryOrder order) volatile;
};

template <Integral T>
class Atomic<T> {
    T m_value{0};

public:
    Atomic() = default;
    Atomic(T value) : m_value(value) {}
    Atomic(const Atomic &) = delete;
    Atomic(Atomic &&) = delete;
    ~Atomic() = default;

    Atomic &operator=(const Atomic &) = delete;
    Atomic &operator=(Atomic &&) = delete;

    // TODO: Declare these out of line.
    T load(MemoryOrder order) const volatile { return __atomic_load_n(&m_value, static_cast<int>(order)); }
    void store(T value, MemoryOrder order) volatile { __atomic_store_n(&m_value, value, static_cast<int>(order)); }

    T fetch_add(T value, MemoryOrder order) volatile {
        return __atomic_fetch_add(&m_value, value, static_cast<int>(order));
    }
    T fetch_sub(T value, MemoryOrder order) volatile {
        return __atomic_fetch_sub(&m_value, value, static_cast<int>(order));
    }
};

template <typename T>
T Atomic<T>::load(MemoryOrder order) const volatile {
    alignas(T) Array<uint8, sizeof(T)> buf;
    auto *ret = reinterpret_cast<T *>(buf.data());
    __atomic_load(&m_value, ret, static_cast<int>(order));
    return *ret;
}

template <typename T>
void Atomic<T>::store(T value, MemoryOrder order) volatile {
    __atomic_store(&m_value, &value, static_cast<int>(order));
}

} // namespace ustd

using ustd::Atomic;
using ustd::MemoryOrder;