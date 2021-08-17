#include <kernel/proc/ThreadBlocker.hh>

#include <kernel/fs/File.hh>
#include <kernel/proc/Process.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Types.hh>

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
