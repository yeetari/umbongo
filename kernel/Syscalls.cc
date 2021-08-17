#include <kernel/proc/Process.hh>

#include <kernel/SysError.hh>
#include <kernel/SysResult.hh>
#include <kernel/SyscallTypes.hh>
#include <kernel/cpu/Processor.hh>
#include <kernel/devices/DevFs.hh>
#include <kernel/fs/File.hh>
#include <kernel/fs/FileHandle.hh>
#include <kernel/fs/FileSystem.hh>
#include <kernel/fs/Pipe.hh>
#include <kernel/fs/Vfs.hh>
#include <kernel/mem/Region.hh>
#include <kernel/mem/VirtSpace.hh>
#include <kernel/proc/Scheduler.hh>
#include <kernel/proc/Thread.hh>
#include <kernel/proc/ThreadBlocker.hh>
#include <ustd/Assert.hh>
#include <ustd/Log.hh>
#include <ustd/Memory.hh>
#include <ustd/Optional.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/String.hh>
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
    m_fds[read_fd].emplace(pipe, AttachDirection::Read);
    fds[0] = read_fd;

    uint32 write_fd = allocate_fd();
    m_fds[write_fd].emplace(pipe, AttachDirection::Write);
    fds[1] = write_fd;
    return 0;
}

SysResult Process::sys_create_process(const char *path, const char **argv, FdPair *copy_fds) {
    auto new_thread = Thread::create_user();
    auto &new_process = new_thread->process();
    new_process.m_fds.grow(m_fds.size());
    for (uint32 i = 0; i < m_fds.size(); i++) {
        if (m_fds[i]) {
            new_process.m_fds[i].emplace(*m_fds[i]);
        }
    }

    // TODO: Proper validation.
    ASSERT(argv != nullptr);
    ASSERT(copy_fds != nullptr);

    Vector<String> args;
    for (usize i = 0;; i++) {
        if (argv[i] == nullptr) {
            break;
        }
        args.push(argv[i]);
    }
    for (usize i = 0;; i++) {
        if (copy_fds[i].parent == 0 && copy_fds[i].child == 0) {
            break;
        }
        auto &fd_pair = copy_fds[i];
        new_process.m_fds.grow(fd_pair.child + 1);
        new_process.m_fds[fd_pair.child].emplace(*m_fds[fd_pair.parent]);
    }

    String copied_path(path);
    if (auto rc = new_thread->exec(copied_path.view(), args); rc.is_error()) {
        return rc;
    }
    Scheduler::insert_thread(ustd::move(new_thread));
    return new_process.pid();
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
    if (code != 0) {
        dbgln("[#{}]: sys_exit called with non-zero code {}", m_pid, code);
    }
    Scheduler::yield_and_kill();
    return 0;
}

SysResult Process::sys_getpid() const {
    return m_pid;
}

SysResult Process::sys_ioctl(uint32 fd, IoctlRequest request, void *arg) {
    if (fd >= m_fds.size() || !m_fds[fd]) {
        return SysError::BadFd;
    }
    return m_fds[fd]->ioctl(request, arg);
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

SysResult Process::sys_open(const char *path, OpenMode mode) {
    // Open file first so we don't leak a file descriptor.
    auto file = Vfs::open(path, mode);
    if (!file) {
        return SysError::NonExistent;
    }
    uint32 fd = allocate_fd();
    m_fds[fd].emplace(ustd::move(file));
    return fd;
}

SysResult Process::sys_putchar(char ch) const {
    dbg_put_char(ch);
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
    auto file = m_fds[fd]->file();
    if (!file->can_read()) {
        Processor::current_thread()->block<ReadBlocker>(file);
    }
    return m_fds[fd]->read(data, size);
}

SysResult Process::sys_seek(uint32 fd, usize offset, SeekMode mode) {
    if (fd >= m_fds.size() || !m_fds[fd]) {
        return SysError::BadFd;
    }
    if (!m_fds[fd]->valid()) {
        m_fds[fd].clear();
        return SysError::BrokenHandle;
    }
    return m_fds[fd]->seek(offset, mode);
}

SysResult Process::sys_size(uint32 fd) {
    if (fd >= m_fds.size() || !m_fds[fd]) {
        return SysError::BadFd;
    }
    if (!m_fds[fd]->valid()) {
        m_fds[fd].clear();
        return SysError::BrokenHandle;
    }
    return m_fds[fd]->size();
}

SysResult Process::sys_wait_pid(usize pid) {
    Processor::current_thread()->block<WaitBlocker>(pid);
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
    auto file = m_fds[fd]->file();
    if (!file->can_write()) {
        Processor::current_thread()->block<WriteBlocker>(file);
    }
    return m_fds[fd]->write(data, size);
}
