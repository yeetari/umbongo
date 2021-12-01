#pragma once

#include <ustd/Atomic.hh>

class SpinLock {
    uint64 m_flags{0};
    ustd::Atomic<bool> m_locked{false};

public:
    void lock() {
        while (m_locked.exchange(true, ustd::MemoryOrder::Acquire)) {
            asm volatile("pause");
        }
        asm volatile("pushf\n\t"
                     "pop %0"
                     : "=rm"(m_flags));
        asm volatile("cli");
    }
    void unlock() {
        m_locked.store(false, ustd::MemoryOrder::Release);
        if ((m_flags & (1u << 9u)) != 0) {
            asm volatile("sti");
        }
    }

    bool locked() const { return m_locked.load(ustd::MemoryOrder::Relaxed); }
};
