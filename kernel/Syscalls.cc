#include <kernel/proc/Process.hh>

#include <kernel/SysError.hh>
#include <kernel/SysResult.hh>
#include <kernel/fs/FileHandle.hh>
#include <kernel/fs/Vfs.hh>
#include <kernel/proc/Scheduler.hh>
#include <ustd/Log.hh>
#include <ustd/Memory.hh>
#include <ustd/Optional.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>
#include <ustd/Vector.hh>

SysResult Process::sys_close(uint32 fd) {
    if (fd >= m_fds.size() || !m_fds[fd]) {
        return SysError::BadFd;
    }
    m_fds[fd].clear();
    return 0;
}

SysResult Process::sys_create_process(const char *path) {
    auto *process = Process::create_user();
    process->m_fds.resize(m_fds.size());
    for (uint32 i = 0; i < m_fds.size(); i++) {
        if (m_fds[i]) {
            process->m_fds[i].emplace(*m_fds[i]);
        }
    }
    process->exec(path);
    Scheduler::insert_process(process);
    return process->pid();
}

SysResult Process::sys_exit(usize code) const {
    logln("[#{}]: sys_exit called with code {}", m_pid, code);
    return 0;
}

SysResult Process::sys_getpid() const {
    return m_pid;
}

SysResult Process::sys_open(const char *path) {
    // Open file first so we don't leak a file descriptor.
    auto file = Vfs::open(path);
    if (!file) {
        return SysError::NonExistent;
    }
    uint32 fd = allocate_fd();
    m_fds[fd].emplace(ustd::move(file));
    return fd;
}

SysResult Process::sys_putchar(char ch) const {
    put_char(ch);
    return 0;
}

SysResult Process::sys_read(uint32 fd, void *data, usize size) const {
    if (fd >= m_fds.size() || !m_fds[fd]) {
        return SysError::BadFd;
    }
    return m_fds[fd]->read(data, size);
}

SysResult Process::sys_write(uint32 fd, void *data, usize size) const {
    if (fd >= m_fds.size() || !m_fds[fd]) {
        return SysError::BadFd;
    }
    return m_fds[fd]->write(data, size);
}
