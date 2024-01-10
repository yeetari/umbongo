#pragma once

#include <ustd/atomic.hh>
#include <ustd/types.hh>

namespace kernel {

class SpinLock {
    ustd::Atomic<uint32_t> m_value{0};

public:
    void lock();
    void unlock();

    bool is_locked() const;
    bool is_locked_by_current_cpu() const;
};

} // namespace kernel
