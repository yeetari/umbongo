#include <kernel/spin_lock.hh>

#include <kernel/arch/cpu.hh>
#include <ustd/assert.hh>

namespace kernel {

void SpinLock::lock() {
    uint32_t desired = arch::current_cpu() + 1u;
    while (!m_value.cmpxchg(0u, desired, ustd::memory_order_acquire)) {
        do {
            arch::cpu_relax();
        } while (m_value.load() != 0u);
    }
}

void SpinLock::unlock() {
    ASSERT(is_locked_by_current_cpu());
    m_value.store(0u, ustd::memory_order_release);
}

bool SpinLock::is_locked() const {
    return m_value.load() != 0u;
}

bool SpinLock::is_locked_by_current_cpu() const {
    return m_value.load() == arch::current_cpu() + 1u;
}

} // namespace kernel
