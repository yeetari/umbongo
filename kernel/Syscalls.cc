#include <kernel/proc/Process.hh>

#include <kernel/fs/FileHandle.hh>
#include <kernel/fs/Vfs.hh>
#include <ustd/Assert.hh>
#include <ustd/Log.hh>
#include <ustd/Memory.hh>
#include <ustd/Optional.hh>
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

uint64 Process::sys_close(uint32 fd) {
    if (fd < m_fds.size()) {
        m_fds[fd].clear();
    }
    return 0;
}

uint64 Process::sys_exit(uint64 code) const {
    logln("[#{}]: sys_exit called with code {}", m_pid, code);
    return 0;
}

uint64 Process::sys_getpid() const {
    return m_pid;
}

uint64 Process::sys_open(const char *path) {
    uint32 fd = allocate_fd();
    m_fds[fd].emplace(Vfs::open(path));
    return fd;
}

uint64 Process::sys_putchar(char ch) const {
    put_char(ch);
    return 0;
}

uint64 Process::sys_read(uint32 fd, void *data, usize size) const {
    ASSERT(m_fds[fd]);
    return m_fds[fd]->read(data, size);
}

uint64 Process::sys_write(uint32 fd, void *data, usize size) const {
    ASSERT(m_fds[fd]);
    return m_fds[fd]->write(data, size);
}
