#include <kernel/proc/thread_blocker.hh>

#include <kernel/api/types.h>
#include <kernel/fs/file.hh>
#include <kernel/fs/file_handle.hh>
#include <kernel/ipc/server_socket.hh>
#include <kernel/ipc/socket.hh>
#include <kernel/proc/process.hh>
#include <kernel/scoped_lock.hh>
#include <kernel/spin_lock.hh>
#include <kernel/time/time_manager.hh>
#include <ustd/optional.hh>
#include <ustd/shared_ptr.hh>
#include <ustd/types.hh>
#include <ustd/vector.hh>

namespace kernel {

bool AcceptBlocker::should_unblock() {
    return !m_server->accept_would_block();
}

bool ConnectBlocker::should_unblock() {
    return m_socket->connected();
}

PollBlocker::PollBlocker(const ustd::LargeVector<ub_poll_fd_t> &fds, SpinLock &lock, Process &process, ssize_t timeout)
    : m_fds(fds), m_lock(lock), m_process(process) {
    if (timeout > 0) {
        m_deadline.emplace(TimeManager::ns_since_boot() + static_cast<size_t>(timeout));
    }
}

bool PollBlocker::should_unblock() {
    if (m_deadline && TimeManager::ns_since_boot() > *m_deadline) {
        return true;
    }

    ScopedLock locker(m_lock);
    for (const auto &poll_fd : m_fds) {
        // TODO: Bounds checking.
        auto &handle = m_process.file_handle(poll_fd.fd);
        if ((poll_fd.events & UB_POLL_EVENT_READ) == UB_POLL_EVENT_READ && !handle.read_would_block()) {
            return true;
        }
        if ((poll_fd.events & UB_POLL_EVENT_WRITE) == UB_POLL_EVENT_WRITE && !handle.write_would_block()) {
            return true;
        }
    }
    return false;
}

bool ReadBlocker::should_unblock() {
    return !m_file->valid() || !m_file->read_would_block(m_offset);
}

WaitBlocker::WaitBlocker(size_t pid) : m_process(Process::from_pid(pid)) {}

bool WaitBlocker::should_unblock() {
    return !m_process || m_process->thread_count() == 0;
}

bool WriteBlocker::should_unblock() {
    return !m_file->valid() || !m_file->write_would_block(m_offset);
}

} // namespace kernel
