#pragma once

#include <ustd/Types.hh>

class InterruptDisabler {
    uint64 m_flags{0};

public:
    InterruptDisabler() {
        asm volatile("pushf\n\t"
                     "pop %0"
                     : "=rm"(m_flags));
        asm volatile("cli");
    }
    InterruptDisabler(const InterruptDisabler &) = delete;
    InterruptDisabler(InterruptDisabler &&) = delete;
    ~InterruptDisabler() {
        if ((m_flags & (1u << 9u)) != 0) {
            asm volatile("sti");
        }
    }

    InterruptDisabler &operator=(const InterruptDisabler &) = delete;
    InterruptDisabler &operator=(InterruptDisabler &&) = delete;
};