#include <kernel/proc/Process.hh>

#include <kernel/SysError.hh>
#include <kernel/SysResult.hh>
#include <kernel/SyscallTypes.hh>
#include <kernel/devices/DevFs.hh>
#include <kernel/fs/FileHandle.hh>
#include <kernel/fs/FileSystem.hh>
#include <kernel/fs/Pipe.hh>
#include <kernel/fs/Vfs.hh>
#include <kernel/mem/Region.hh>
#include <kernel/mem/VirtSpace.hh>
#include <kernel/proc/Scheduler.hh>
#include <ustd/Assert.hh>
#include <ustd/Log.hh>
#include <ustd/Memory.hh>
#include <ustd/Optional.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>
#include <ustd/Utility.hh>
#include <ustd/Vector.hh>

SysResult Process::sys_allocate_region(usize size, MemoryProt prot) {
    auto access = RegionAccess::UserAccessible;
    if ((prot & MemoryProt::Write) == MemoryProt::Write) {
        access |= RegionAccess::Writable;
    }
    if ((prot & MemoryProt::Exec) == MemoryProt::Exec) {
        access |= RegionAccess::Executable;
    }
    auto &region = m_virt_space->allocate_region(size, access);
    return region.base();
}

SysResult Process::sys_close(uint32 fd) {
    if (fd >= m_fds.size() || !m_fds[fd]) {
        return SysError::BadFd;
    }
    m_fds[fd].clear();
    return 0;
}

SysResult Process::sys_create_pipe(uint32 *fds) {
    auto pipe = ustd::make_shared<Pipe>();
    uint32 read_fd = allocate_fd();
    m_fds[read_fd].emplace(pipe);
    fds[0] = read_fd;

    uint32 write_fd = allocate_fd();
    m_fds[write_fd].emplace(pipe);
    fds[1] = write_fd;
    return 0;
}

SysResult Process::sys_create_process(const char *path) {
    auto *process = Process::create_user();
    process->m_fds.grow(m_fds.size());
    for (uint32 i = 0; i < m_fds.size(); i++) {
        if (m_fds[i]) {
            process->m_fds[i].emplace(*m_fds[i]);
        }
    }
    process->exec(path);
    Scheduler::insert_process(process);
    return process->pid();
}

SysResult Process::sys_dup_fd(uint32 src, uint32 dst) {
    // TODO: Which check should happen first?
    if (src >= m_fds.size() || !m_fds[src]) {
        return SysError::BadFd;
    }
    if (src == dst) {
        return 0;
    }
    m_fds.grow(dst + 1);
    m_fds[dst].emplace(*m_fds[src]);
    return 0;
}

SysResult Process::sys_exit(usize code) const {
    logln("[#{}]: sys_exit called with code {}", m_pid, code);
    return 0;
}

SysResult Process::sys_getpid() const {
    return m_pid;
}

SysResult Process::sys_mkdir(const char *path) const {
    Vfs::mkdir(path);
    return 0;
}

SysResult Process::sys_mmap(uint32 fd) const {
    if (fd >= m_fds.size() || !m_fds[fd]) {
        return SysError::BadFd;
    }
    return m_fds[fd]->mmap(*m_virt_space);
}

SysResult Process::sys_mount(const char *target, const char *fs_type) const {
    UniquePtr<FileSystem> fs;
    if (StringView(fs_type) == "dev") {
        fs = ustd::make_unique<DevFs>();
    } else {
        ENSURE_NOT_REACHED();
    }
    Vfs::mount(target, ustd::move(fs));
    return 0;
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

SysResult Process::sys_read(uint32 fd, void *data, usize size) {
    if (fd >= m_fds.size() || !m_fds[fd]) {
        return SysError::BadFd;
    }
    if (!m_fds[fd]->valid()) {
        m_fds[fd].clear();
        return SysError::BrokenHandle;
    }
    return m_fds[fd]->read(data, size);
}

SysResult Process::sys_seek(uint32 fd, usize offset) {
    if (fd >= m_fds.size() || !m_fds[fd]) {
        return SysError::BadFd;
    }
    if (!m_fds[fd]->valid()) {
        m_fds[fd].clear();
        return SysError::BrokenHandle;
    }
    m_fds[fd]->seek(offset);
    return 0;
}

SysResult Process::sys_write(uint32 fd, void *data, usize size) {
    if (fd >= m_fds.size() || !m_fds[fd]) {
        return SysError::BadFd;
    }
    if (!m_fds[fd]->valid()) {
        m_fds[fd].clear();
        return SysError::BrokenHandle;
    }
    return m_fds[fd]->write(data, size);
}
