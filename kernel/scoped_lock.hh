#pragma once

namespace kernel {

template <typename MutexType>
class ScopedLock {
    MutexType *m_mutex;

public:
    explicit ScopedLock(MutexType &mutex) : m_mutex(&mutex) { mutex.lock(); }
    ScopedLock(const ScopedLock &) = delete;
    ScopedLock(ScopedLock &&) = delete;
    ~ScopedLock() {
        if (m_mutex != nullptr) {
            m_mutex->unlock();
        }
    }

    ScopedLock &operator=(const ScopedLock &) = delete;
    ScopedLock &operator=(ScopedLock &&) = delete;

    void unlock() {
        m_mutex->unlock();
        m_mutex = nullptr;
    }
};

template <typename MutexType>
ScopedLock(MutexType) -> ScopedLock<MutexType>;

} // namespace kernel
