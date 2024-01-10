#pragma once

#include <ustd/atomic.hh>

namespace kernel {

class SpinLock {
    ustd::Atomic<bool> m_locked{false};

public:
    void lock() {
        while (m_locked.exchange(true, ustd::memory_order_relaxed)) {
            asm volatile("pause");
        }
    }

    void unlock() { m_locked.store(false, ustd::memory_order_relaxed); }
    bool locked() const { return m_locked.load(ustd::memory_order_relaxed); }
};

} // namespace kernel
