#pragma once

#include <ustd/Array.hh>
#include <ustd/Concepts.hh>
#include <ustd/Traits.hh>
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

    T exchange(T desired, MemoryOrder order) volatile;
    T load(MemoryOrder order) const volatile;
    void store(T value, MemoryOrder order) volatile;
};

template <typename T>
struct SimpleAtomicStorageType {
    using type = T;
};
template <Enum T>
struct SimpleAtomicStorageType<T> {
    using type = underlying_type<T>;
};
template <typename T>
using simple_atomic_storage_t = typename SimpleAtomicStorageType<T>::type;

template <typename T>
concept SimpleAtomic = requires(simple_atomic_storage_t<T> t) { __atomic_load_n(&t, __ATOMIC_RELAXED); };

template <SimpleAtomic T>
class Atomic<T> {
    using storage_t = simple_atomic_storage_t<T>;
    storage_t m_value{};

public:
    Atomic() = default;
    Atomic(T value) : m_value(storage_t(value)) {}
    Atomic(const Atomic &) = delete;
    Atomic(Atomic &&) = delete;
    ~Atomic() = default;

    Atomic &operator=(const Atomic &) = delete;
    Atomic &operator=(Atomic &&) = delete;

    // TODO: Declare these out of line when clang supports it.
    bool compare_exchange(T &expected, T desired, MemoryOrder success_order = MemoryOrder::Relaxed,
                          MemoryOrder failure_order = MemoryOrder::Relaxed) {
        return __atomic_compare_exchange_n(&m_value, reinterpret_cast<storage_t *>(&expected), storage_t(desired),
                                           false, static_cast<int>(success_order), static_cast<int>(failure_order));
    }

    bool compare_exchange_temp(T expected, T desired, MemoryOrder success_order = MemoryOrder::Relaxed,
                               MemoryOrder failure_order = MemoryOrder::Relaxed) {
        return __atomic_compare_exchange_n(&m_value, reinterpret_cast<storage_t *>(&expected), storage_t(desired),
                                           false, static_cast<int>(success_order), static_cast<int>(failure_order));
    }

    T exchange(T desired, MemoryOrder order = MemoryOrder::Relaxed) volatile {
        return T(__atomic_exchange_n(&m_value, storage_t(desired), static_cast<int>(order)));
    }

    T fetch_add(T value, MemoryOrder order = MemoryOrder::Relaxed) volatile {
        return __atomic_fetch_add(&m_value, value, static_cast<int>(order));
    }

    T fetch_sub(T value, MemoryOrder order = MemoryOrder::Relaxed) volatile {
        return __atomic_fetch_sub(&m_value, value, static_cast<int>(order));
    }

    T load(MemoryOrder order = MemoryOrder::Relaxed) const volatile {
        return T(__atomic_load_n(&m_value, static_cast<int>(order)));
    }

    void store(T value, MemoryOrder order = MemoryOrder::Relaxed) volatile {
        __atomic_store_n(&m_value, storage_t(value), static_cast<int>(order));
    }

    storage_t *raw_ptr() { return &m_value; }
};

template <typename T>
T Atomic<T>::exchange(T desired, MemoryOrder order) volatile {
    alignas(T) Array<uint8_t, sizeof(T)> buf;
    auto *ret = reinterpret_cast<T *>(buf.data());
    __atomic_exchange(&m_value, &desired, ret, static_cast<int>(order));
    return *ret;
}

template <typename T>
T Atomic<T>::load(MemoryOrder order) const volatile {
    alignas(T) Array<uint8_t, sizeof(T)> buf;
    auto *ret = reinterpret_cast<T *>(buf.data());
    __atomic_load(&m_value, ret, static_cast<int>(order));
    return *ret;
}

template <typename T>
void Atomic<T>::store(T value, MemoryOrder order) volatile {
    __atomic_store(&m_value, &value, static_cast<int>(order));
}

} // namespace ustd
