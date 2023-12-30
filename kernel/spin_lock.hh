#pragma once

#include <ustd/atomic.hh>

namespace kernel {

class SpinLock {
    ustd::Atomic<bool> m_locked{false};

public:
    void lock() {
        while (m_locked.exchange(true, ustd::MemoryOrder::Relaxed)) {
            asm volatile("pause");
        }
    }

    void unlock() { m_locked.store(false, ustd::MemoryOrder::Relaxed); }
    bool locked() const { return m_locked.load(ustd::MemoryOrder::Relaxed); }
};

} // namespace kernel
