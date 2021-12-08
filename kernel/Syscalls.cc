#include <kernel/proc/Process.hh>

#include <kernel/ScopedLock.hh> // IWYU pragma: keep
#include <kernel/SysError.hh>
#include <kernel/SysResult.hh>
#include <kernel/SyscallTypes.hh>
#include <kernel/cpu/Processor.hh>
#include <kernel/fs/File.hh>
#include <kernel/fs/FileHandle.hh>
#include <kernel/fs/FileSystem.hh>
#include <kernel/fs/Inode.hh>
#include <kernel/fs/InodeFile.hh>
#include <kernel/fs/InodeType.hh>
#include <kernel/fs/RamFs.hh>
#include <kernel/fs/Vfs.hh>
#include <kernel/ipc/DoubleBuffer.hh>
#include <kernel/ipc/Pipe.hh>
#include <kernel/ipc/ServerSocket.hh>
#include <kernel/ipc/Socket.hh>
#include <kernel/mem/Region.hh>
#include <kernel/mem/VirtSpace.hh>
#include <kernel/proc/Scheduler.hh>
#include <kernel/proc/Thread.hh>
#include <kernel/proc/ThreadBlocker.hh>
#include <kernel/time/TimeManager.hh>
#include <ustd/Assert.hh>
#include <ustd/Log.hh>
#include <ustd/Memory.hh>
#include <ustd/Optional.hh>
#include <ustd/Result.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/String.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>
#include <ustd/Utility.hh>
#include <ustd/Vector.hh>

SyscallResult Process::sys_accept(uint32 fd) {
    ScopedLock locker(m_lock);
    if (fd >= m_fds.size() || !m_fds[fd]) {
        return SysError::BadFd;
    }
    auto &file = m_fds[fd]->file();
    if (!file.is_server_socket()) {
        return SysError::Invalid;
    }
    auto &server = static_cast<ServerSocket &>(file);
    if (!server.can_accept()) {
        Processor::current_thread()->block<AcceptBlocker>(ustd::SharedPtr<ServerSocket>(&server));
    }
    auto accepted = server.accept();
    uint32 accepted_fd = allocate_fd();
    m_fds[accepted_fd].emplace(ustd::move(accepted));
    return accepted_fd;
}

SyscallResult Process::sys_allocate_region(usize size, MemoryProt prot) {
    ScopedLock locker(m_lock);
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

SyscallResult Process::sys_chdir(const char *path) {
    ScopedLock locker(m_lock);
    auto directory = Vfs::open_directory(path, m_cwd);
    if (directory.is_error()) {
        return directory.error();
    }
    m_cwd = *directory;
    return 0;
}

SyscallResult Process::sys_close(uint32 fd) {
    ScopedLock locker(m_lock);
    if (fd >= m_fds.size() || !m_fds[fd]) {
        return SysError::BadFd;
    }
    m_fds[fd].clear();
    return 0;
}

SyscallResult Process::sys_connect(const char *path) {
    auto file_or_error = Vfs::open(path, OpenMode::None, m_cwd);
    if (file_or_error.is_error()) {
        return file_or_error.error();
    }
    auto &file = *file_or_error;
    if (!file->is_server_socket()) {
        return SysError::Invalid;
    }
    auto client = ustd::make_shared<Socket>(new DoubleBuffer(64_KiB), new DoubleBuffer(64_KiB));
    auto &server = static_cast<ServerSocket &>(*file);
    if (auto rc = server.queue_connection_from(client); rc.is_error()) {
        return rc.error();
    }
    Processor::current_thread()->block<ConnectBlocker>(client);
    ScopedLock locker(m_lock);
    uint32 client_fd = allocate_fd();
    m_fds[client_fd].emplace(client);
    return client_fd;
}

SyscallResult Process::sys_create_pipe(uint32 *fds) {
    ScopedLock locker(m_lock);
    auto pipe = ustd::make_shared<Pipe>();
    uint32 read_fd = allocate_fd();
    m_fds[read_fd].emplace(pipe, AttachDirection::Read);
    fds[0] = read_fd;

    uint32 write_fd = allocate_fd();
    m_fds[write_fd].emplace(pipe, AttachDirection::Write);
    fds[1] = write_fd;
    return 0;
}

SyscallResult Process::sys_create_process(const char *path, const char **argv, FdPair *copy_fds) {
    ScopedLock locker(m_lock);
    auto new_thread = Thread::create_user();
    auto &new_process = new_thread->process();
    new_process.m_cwd = m_cwd;
    new_process.m_fds.ensure_size(m_fds.size());
    for (uint32 i = 0; i < m_fds.size(); i++) {
        if (m_fds[i]) {
            new_process.m_fds[i].emplace(*m_fds[i]);
        }
    }

    // TODO: Proper validation.
    ASSERT(argv != nullptr);
    ASSERT(copy_fds != nullptr);

    ustd::Vector<ustd::String> args;
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
        new_process.m_fds.ensure_size(fd_pair.child + 1);
        new_process.m_fds[fd_pair.child].emplace(*m_fds[fd_pair.parent]);
    }

    ustd::String copied_path(path);
    if (auto rc = new_thread->exec(copied_path.view(), args); rc.is_error()) {
        return rc.error();
    }
    Scheduler::insert_thread(ustd::move(new_thread));
    return new_process.pid();
}

// TODO: A bind syscall - create_pipe and create_server_socket both return anonymous files, that can be bound to inodes.
SyscallResult Process::sys_create_server_socket(const char *path, uint32 backlog_limit) {
    // TODO: Upper limit for backlog_limit.
    auto inode = Vfs::create(path, m_cwd, InodeType::IpcFile);
    if (inode.is_error()) {
        return inode.error();
    }
    ScopedLock locker(m_lock);
    auto server = ustd::make_shared<ServerSocket>(backlog_limit);
    uint32 fd = allocate_fd();
    m_fds[fd].emplace(server);
    inode->bind_ipc_file(server);
    return fd;
}

SyscallResult Process::sys_dup_fd(uint32 src, uint32 dst) {
    // TODO: Which check should happen first?
    ScopedLock locker(m_lock);
    if (src >= m_fds.size() || !m_fds[src]) {
        return SysError::BadFd;
    }
    if (src == dst) {
        return 0;
    }
    m_fds.ensure_size(dst + 1);
    m_fds[dst].emplace(*m_fds[src]);
    return 0;
}

SyscallResult Process::sys_exit(usize code) const {
    if (code != 0) {
        ustd::dbgln("[#{}]: sys_exit called with non-zero code {}", m_pid, code);
    }
    Scheduler::yield_and_kill();
    return 0;
}

SyscallResult Process::sys_getcwd(char *path) const {
    ScopedLock locker(m_lock);
    ustd::Vector<Inode *> inodes;
    for (auto *inode = m_cwd; inode != Vfs::root_inode(); inode = inode->parent()) {
        inodes.push(inode);
    }
    // TODO: Would be nice to have some kind of ustd::reverse_iterator(inodes) function.
    ustd::Vector<char> path_characters;
    for (uint32 i = inodes.size(); i-- != 0;) {
        path_characters.push('/');
        for (auto ch : inodes[i]->name()) {
            path_characters.push(ch);
        }
    }
    if (path_characters.empty()) {
        path_characters.push('/');
    }
    if (path == nullptr) {
        return path_characters.size();
    }
    memcpy(path, path_characters.data(), path_characters.size());
    return 0;
}

SyscallResult Process::sys_getpid() const {
    return m_pid;
}

SyscallResult Process::sys_gettime() const {
    return TimeManager::ns_since_boot();
}

SyscallResult Process::sys_ioctl(uint32 fd, IoctlRequest request, void *arg) {
    ScopedLock locker(m_lock);
    if (fd >= m_fds.size() || !m_fds[fd]) {
        return SysError::BadFd;
    }
    return m_fds[fd]->ioctl(request, arg);
}

SyscallResult Process::sys_mkdir(const char *path) const {
    ScopedLock locker(m_lock);
    return Vfs::mkdir(path, m_cwd);
}

SyscallResult Process::sys_mmap(uint32 fd) const {
    ScopedLock locker(m_lock);
    if (fd >= m_fds.size() || !m_fds[fd]) {
        return SysError::BadFd;
    }
    return m_fds[fd]->mmap(*m_virt_space);
}

SyscallResult Process::sys_mount(const char *target, const char *fs_type) const {
    ScopedLock locker(m_lock);
    ustd::UniquePtr<FileSystem> fs;
    if (ustd::StringView(fs_type) == "ram") {
        fs = ustd::make_unique<RamFs>();
    } else {
        return SysError::Invalid;
    }
    return Vfs::mount(target, ustd::move(fs));
}

SyscallResult Process::sys_open(const char *path, OpenMode mode) {
    ScopedLock locker(m_lock);
    // Open file first so that we don't leak a file descriptor.
    auto file = Vfs::open(path, mode, m_cwd);
    if (file.is_error()) {
        return file.error();
    }
    uint32 fd = allocate_fd();
    m_fds[fd].emplace(ustd::move(*file));
    return fd;
}

SyscallResult Process::sys_poll(PollFd *fds, usize count, ssize timeout) {
    ustd::LargeVector<PollFd> poll_fds(count);
    memcpy(poll_fds.data(), fds, count * sizeof(PollFd));
    Processor::current_thread()->block<PollBlocker>(poll_fds, m_lock, *this, timeout);
    for (auto &poll_fd : poll_fds) {
        // TODO: Bounds checking.
        auto &file = m_fds[poll_fd.fd]->file();
        poll_fd.revents = static_cast<PollEvents>(0);
        if ((poll_fd.events & PollEvents::Read) == PollEvents::Read && file.can_read()) {
            poll_fd.revents |= PollEvents::Read;
        }
        if ((poll_fd.events & PollEvents::Write) == PollEvents::Write && file.can_write()) {
            poll_fd.revents |= PollEvents::Write;
        }
    }
    memcpy(fds, poll_fds.data(), count * sizeof(PollFd));
    return 0;
}

SyscallResult Process::sys_putchar(char ch) const {
    dbg_put_char(ch);
    return 0;
}

SyscallResult Process::sys_read(uint32 fd, void *data, usize size) {
    ScopedLock locker(m_lock);
    if (fd >= m_fds.size() || !m_fds[fd]) {
        return SysError::BadFd;
    }
    auto &handle = m_fds[fd];
    if (!handle->valid()) {
        handle.clear();
        return SysError::BrokenHandle;
    }
    auto &file = handle->file();
    if (!file.can_read()) {
        Processor::current_thread()->block<ReadBlocker>(file);
    }
    auto bytes_read = handle->read(data, size);
    if (bytes_read.is_error()) {
        return bytes_read.error();
    }
    return *bytes_read;
}

SyscallResult Process::sys_read_directory(const char *path, uint8 *data) {
    ScopedLock locker(m_lock);
    auto directory = Vfs::open_directory(path, m_cwd);
    if (directory.is_error()) {
        return directory.error();
    }
    if (data == nullptr) {
        usize byte_count = 0;
        for (usize i = 0; i < directory->size(); i++) {
            auto *child = directory->child(i);
            byte_count += child->name().length() + 1;
        }
        return byte_count;
    }
    usize byte_offset = 0;
    for (usize i = 0; i < directory->size(); i++) {
        auto *child = directory->child(i);
        memcpy(data + byte_offset, child->name().data(), child->name().length());
        data[byte_offset + child->name().length()] = '\0';
        byte_offset += child->name().length() + 1;
    }
    return 0;
}

SyscallResult Process::sys_seek(uint32 fd, usize offset, SeekMode mode) {
    ScopedLock locker(m_lock);
    if (fd >= m_fds.size() || !m_fds[fd]) {
        return SysError::BadFd;
    }
    if (!m_fds[fd]->valid()) {
        m_fds[fd].clear();
        return SysError::BrokenHandle;
    }
    return m_fds[fd]->seek(offset, mode);
}

SyscallResult Process::sys_size(uint32 fd) {
    ScopedLock locker(m_lock);
    if (fd >= m_fds.size() || !m_fds[fd]) {
        return SysError::BadFd;
    }
    if (!m_fds[fd]->valid()) {
        m_fds[fd].clear();
        return SysError::BrokenHandle;
    }
    auto &file = m_fds[fd]->file();
    if (!file.is_inode_file()) {
        return SysError::Invalid;
    }
    return static_cast<InodeFile &>(file).inode()->size();
}

SyscallResult Process::sys_wait_pid(usize pid) {
    Processor::current_thread()->block<WaitBlocker>(pid);
    return 0;
}

SyscallResult Process::sys_write(uint32 fd, void *data, usize size) {
    ScopedLock locker(m_lock);
    if (fd >= m_fds.size() || !m_fds[fd]) {
        return SysError::BadFd;
    }
    auto &handle = m_fds[fd];
    if (!handle->valid()) {
        handle.clear();
        return SysError::BrokenHandle;
    }
    auto &file = handle->file();
    if (!file.can_write()) {
        Processor::current_thread()->block<WriteBlocker>(file);
    }
    auto bytes_written = handle->write(data, size);
    if (bytes_written.is_error()) {
        return bytes_written.error();
    }
    return *bytes_written;
}
