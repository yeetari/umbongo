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

bool AcceptBlocker::should_unblock() {
    return m_server->can_accept();
}

bool ConnectBlocker::should_unblock() {
    return m_socket->connected();
}

PollBlocker::PollBlocker(const ustd::LargeVector<PollFd> &fds, SpinLock &lock, Process &process, ssize timeout)
    : m_fds(fds), m_lock(lock), m_process(process) {
    if (timeout > 0) {
        m_deadline.emplace(TimeManager::ns_since_boot() + static_cast<usize>(timeout));
    }
}

bool PollBlocker::should_unblock() {
    if (m_deadline && TimeManager::ns_since_boot() > *m_deadline) {
        return true;
    }

    ScopedLock locker(m_lock);
    for (const auto &poll_fd : m_fds) {
        // TODO: Bounds checking.
        auto &file = m_process.file_handle(poll_fd.fd).file();
        if ((poll_fd.events & PollEvents::Read) == PollEvents::Read && file.can_read()) {
            return true;
        }
        if ((poll_fd.events & PollEvents::Write) == PollEvents::Write && file.can_write()) {
            return true;
        }
    }
    return false;
}

bool ReadBlocker::should_unblock() {
    return !m_file->valid() || m_file->can_read();
}

WaitBlocker::WaitBlocker(usize pid) : m_process(Process::from_pid(pid)) {}

bool WaitBlocker::should_unblock() {
    return !m_process || m_process->thread_count() == 0;
}

bool WriteBlocker::should_unblock() {
    return !m_file->valid() || m_file->can_write();
}
