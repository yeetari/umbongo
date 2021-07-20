#pragma once

#include <ustd/Assert.hh>
#include <ustd/Atomic.hh>
#include <ustd/Concepts.hh>
#include <ustd/Types.hh>

namespace ustd {

template <typename Derived, Integral RefCountType = uint32>
class Shareable {
    template <typename T>
    friend class SharedPtr;

private:
    mutable Atomic<RefCountType> m_ref_count{1};

    void add_ref() const { m_ref_count.fetch_add(1, MemoryOrder::Relaxed); }
    void sub_ref() const {
        if (m_ref_count.fetch_sub(1, MemoryOrder::AcqRel) == 1) {
            delete static_cast<const Derived *>(this);
        }
    }

public:
    Shareable() = default;
    Shareable(const Shareable &) = delete;
    Shareable(Shareable &&) = delete;
    ~Shareable() { ASSERT(m_ref_count.load(MemoryOrder::Relaxed) == 0); }

    Shareable &operator=(const Shareable &) = delete;
    Shareable &operator=(Shareable &&) = delete;
};

} // namespace ustd

using ustd::Shareable;
