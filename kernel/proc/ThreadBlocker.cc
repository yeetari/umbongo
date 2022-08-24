#include <kernel/proc/ThreadBlocker.hh>

#include <kernel/ScopedLock.hh> // IWYU pragma: keep
#include <kernel/SyscallTypes.hh>
#include <kernel/fs/File.hh>
#include <kernel/fs/FileHandle.hh>
#include <kernel/ipc/ServerSocket.hh>
#include <kernel/ipc/Socket.hh>
#include <kernel/proc/Process.hh>
#include <kernel/time/TimeManager.hh>
#include <ustd/Optional.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

namespace kernel {

bool AcceptBlocker::should_unblock() {
    return !m_server->accept_would_block();
}

bool ConnectBlocker::should_unblock() {
    return m_socket->connected();
}

PollBlocker::PollBlocker(const ustd::LargeVector<PollFd> &fds, SpinLock &lock, Process &process, ssize_t timeout)
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
        if ((poll_fd.events & PollEvents::Read) == PollEvents::Read && !handle.read_would_block()) {
            return true;
        }
        if ((poll_fd.events & PollEvents::Write) == PollEvents::Write && !handle.write_would_block()) {
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
