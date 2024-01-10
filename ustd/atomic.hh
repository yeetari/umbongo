#pragma once

#include <ustd/utility.hh>

namespace ustd {

inline constexpr int memory_order_relaxed = __ATOMIC_RELAXED;
inline constexpr int memory_order_acquire = __ATOMIC_ACQUIRE;
inline constexpr int memory_order_release = __ATOMIC_RELEASE;
inline constexpr int memory_order_acq_rel = __ATOMIC_ACQ_REL;
inline constexpr int memory_order_seq_cst = __ATOMIC_SEQ_CST;

template <typename T>
struct AtomicStorageType {
    using type = T;
};
template <Enum T>
struct AtomicStorageType<T> {
    using type = underlying_type<T>;
};

template <typename T>
using atomic_storage_t = typename AtomicStorageType<T>::type;

template <typename T>
concept SimpleAtomic = requires(atomic_storage_t<T> t) { __atomic_load_n(&t, __ATOMIC_RELAXED); };

template <SimpleAtomic T, int Order = memory_order_relaxed>
class Atomic {
    using storage_t = atomic_storage_t<T>;
    storage_t m_value{};

public:
    Atomic() = default;
    Atomic(T value) : m_value(storage_t(value)) {}
    Atomic(const Atomic &) = delete;
    Atomic(Atomic &&) = delete;
    ~Atomic() = default;

    Atomic &operator=(const Atomic &) = delete;
    Atomic &operator=(Atomic &&) = delete;

    bool cmpxchg(T expected, T desired, int success_order = Order, int failure_order = memory_order_relaxed) volatile;
    bool compare_exchange(T &expected, T desired, int success_order = Order,
                          int failure_order = memory_order_relaxed) volatile;
    T exchange(T desired, int order = Order) volatile;
    T fetch_add(T value, int order = Order) volatile;
    T fetch_sub(T value, int order = Order) volatile;
    T fetch_and(T value, int order = Order) volatile;
    T fetch_or(T value, int order = Order) volatile;
    T fetch_xor(T value, int order = Order) volatile;
    T load(int order = Order) const volatile;
    void store(T value, int order = Order) volatile;

    storage_t *raw_ptr() { return &m_value; }
};

template <SimpleAtomic T, int Order>
bool Atomic<T, Order>::cmpxchg(T expected, T desired, int success_order, int failure_order) volatile {
    return __atomic_compare_exchange_n(&m_value, &expected, storage_t(desired), false, success_order, failure_order);
}

template <SimpleAtomic T, int Order>
bool Atomic<T, Order>::compare_exchange(T &expected, T desired, int success_order, int failure_order) volatile {
    return __atomic_compare_exchange_n(&m_value, &expected, storage_t(desired), false, success_order, failure_order);
}

template <SimpleAtomic T, int Order>
T Atomic<T, Order>::exchange(T desired, int order) volatile {
    return T(__atomic_exchange_n(&m_value, storage_t(desired), order));
}

template <SimpleAtomic T, int Order>
T Atomic<T, Order>::fetch_add(T value, int order) volatile {
    return __atomic_fetch_add(&m_value, value, order);
}

template <SimpleAtomic T, int Order>
T Atomic<T, Order>::fetch_sub(T value, int order) volatile {
    return __atomic_fetch_sub(&m_value, value, order);
}

template <SimpleAtomic T, int Order>
T Atomic<T, Order>::fetch_and(T value, int order) volatile {
    return __atomic_fetch_and(&m_value, value, order);
}

template <SimpleAtomic T, int Order>
T Atomic<T, Order>::fetch_or(T value, int order) volatile {
    return __atomic_fetch_or(&m_value, value, order);
}

template <SimpleAtomic T, int Order>
T Atomic<T, Order>::fetch_xor(T value, int order) volatile {
    return __atomic_fetch_xor(&m_value, value, order);
}

template <SimpleAtomic T, int Order>
T Atomic<T, Order>::load(int order) const volatile {
    return T(__atomic_load_n(&m_value, order));
}

template <SimpleAtomic T, int Order>
void Atomic<T, Order>::store(T value, int order) volatile {
    __atomic_store_n(&m_value, storage_t(value), order);
}

inline void atomic_thread_fence(int order) {
    __atomic_thread_fence(order);
}

} // namespace ustd
