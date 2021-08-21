#pragma once

#include <ustd/Atomic.hh>

class SpinLock {
    Atomic<bool> m_locked{false};

public:
    void lock() {
        while (m_locked.exchange(true, MemoryOrder::Acquire)) {
            asm volatile("pause");
        }
    }
    void unlock() { m_locked.store(false, MemoryOrder::Release); }

    bool locked() const { return m_locked.load(MemoryOrder::Relaxed); }
};
