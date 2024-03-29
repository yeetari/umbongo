#pragma once

#include <ustd/assert.hh>
#include <ustd/atomic.hh>
#include <ustd/numeric.hh>
#include <ustd/types.hh>

namespace ustd {

template <typename Derived, Integral RefCountType = uint32_t>
class Shareable {
    template <typename T>
    friend class SharedPtr;

private:
    mutable Atomic<RefCountType> m_ref_count{0};

    void add_ref() const { m_ref_count.fetch_add(1, memory_order_relaxed); }
    void sub_ref() const {
        if (m_ref_count.fetch_sub(1, memory_order_acq_rel) == 1) {
            delete static_cast<const Derived *>(this);
        }
    }

public:
    Shareable() = default;
    Shareable(const Shareable &) = delete;
    Shareable(Shareable &&) = delete;
    ~Shareable() { ASSERT(m_ref_count.load(memory_order_relaxed) == 0); }

    Shareable &operator=(const Shareable &) = delete;
    Shareable &operator=(Shareable &&) = delete;

    void leak_ref() { const_cast<Shareable *>(this)->add_ref(); }

    RefCountType ref_count() const { return m_ref_count.load(memory_order_relaxed); }
};

} // namespace ustd
