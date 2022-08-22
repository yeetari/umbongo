#pragma once

#include <ustd/Assert.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>

template <typename I>
class InodePtr {
    I *m_inode{nullptr};
    usize *m_ref_count{nullptr};

public:
    constexpr InodePtr() = default;
    InodePtr(I *inode) : m_inode(inode), m_ref_count(new usize(1)) {}
    InodePtr(const InodePtr &other) : m_inode(other.m_inode), m_ref_count(other.m_ref_count) { (*m_ref_count)++; }
    InodePtr(InodePtr &&other)
        : m_inode(ustd::exchange(other.m_inode, nullptr)), m_ref_count(ustd::exchange(other.m_ref_count, nullptr)) {}
    ~InodePtr() {
        if (m_ref_count != nullptr && --(*m_ref_count) == 0) {
            delete ustd::exchange(m_inode, nullptr);
            delete ustd::exchange(m_ref_count, nullptr);
        }
    }

    InodePtr &operator=(const InodePtr &) = delete;
    InodePtr &operator=(InodePtr &&other) {
        InodePtr ptr(ustd::move(other));
        ustd::swap(m_inode, ptr.m_inode);
        ustd::swap(m_ref_count, ptr.m_ref_count);
        return *this;
    }

    I &operator*() const {
        ASSERT(m_inode != nullptr);
        return *m_inode;
    }
    I *operator->() const {
        ASSERT(m_inode != nullptr);
        return m_inode;
    }

    usize ref_count() const { return *m_ref_count; }
};
