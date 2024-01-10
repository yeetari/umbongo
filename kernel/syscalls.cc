#include <kernel/proc/process.hh>

#include <kernel/api/types.h>
#include <kernel/dmesg.hh>
#include <kernel/error.hh>
#include <kernel/fs/file.hh>
#include <kernel/fs/file_handle.hh>
#include <kernel/fs/file_system.hh>
#include <kernel/fs/inode.hh>
#include <kernel/fs/inode_file.hh>
#include <kernel/fs/ram_fs.hh>
#include <kernel/fs/vfs.hh>
#include <kernel/ipc/double_buffer.hh>
#include <kernel/ipc/pipe.hh>
#include <kernel/ipc/server_socket.hh>
#include <kernel/ipc/socket.hh>
#include <kernel/mem/address_space.hh>
#include <kernel/mem/physical_page.hh>
#include <kernel/mem/region.hh>
#include <kernel/mem/vm_object.hh>
#include <kernel/proc/scheduler.hh>
#include <kernel/proc/thread.hh>
#include <kernel/proc/thread_blocker.hh>
#include <kernel/scoped_lock.hh>
#include <kernel/spin_lock.hh>
#include <kernel/sys_result.hh>
#include <kernel/time/time_manager.hh>
#include <ustd/assert.hh>
#include <ustd/optional.hh>
#include <ustd/shared_ptr.hh>
#include <ustd/string.hh>
#include <ustd/string_view.hh>
#include <ustd/try.hh>
#include <ustd/types.hh>
#include <ustd/unique_ptr.hh>
#include <ustd/utility.hh>
#include <ustd/vector.hh>

namespace kernel {

SyscallResult Process::sys_accept(uint32_t fd) {
    ScopedLock lock(m_lock);
    if (fd >= m_fds.size() || !m_fds[fd]) {
        return Error::BadFd;
    }
    auto &file = m_fds[fd]->file();
    if (!file.is_server_socket()) {
        return Error::Invalid;
    }

    // Release lock before potentially blocking.
    lock.unlock();

    auto &server = static_cast<ServerSocket &>(file);
    if (server.accept_would_block()) {
        Thread::current().block<AcceptBlocker>(ustd::SharedPtr<ServerSocket>(&server));
    }
    auto accepted = server.accept();

    lock.relock(m_lock);
    uint32_t accepted_fd = allocate_fd();
    m_fds[accepted_fd].emplace(ustd::move(accepted));
    return accepted_fd;
}

SyscallResult Process::sys_allocate_region(size_t size, ub_memory_prot_t prot) {
    size = ustd::align_up(size, 4_KiB);

    auto access = RegionAccess::UserAccessible;
    if ((prot & UB_MEMORY_PROT_WRITE) == UB_MEMORY_PROT_WRITE) {
        access |= RegionAccess::Writable;
    }
    if ((prot & UB_MEMORY_PROT_EXEC) == UB_MEMORY_PROT_EXEC) {
        access |= RegionAccess::Executable;
    }
    if ((prot & UB_MEMORY_PROT_UNCACHEABLE) == UB_MEMORY_PROT_UNCACHEABLE) {
        access |= RegionAccess::Uncacheable;
    }

    auto vm_object = VmObject::create(size);
    auto &region = TRY(m_address_space->allocate_anywhere(size, access));
    region.map(ustd::move(vm_object));
    return region.base();
}

SyscallResult Process::sys_bind(uint32_t fd, const char *path) {
    auto *inode = TRY(Vfs::create(path, m_cwd, InodeType::AnonymousFile));
    ScopedLock lock(m_lock);
    inode->bind_anonymous_file(ustd::SharedPtr<File>(&m_fds[fd]->file()));
    return fd;
}

SyscallResult Process::sys_chdir(const char *path) {
    ScopedLock lock(m_lock);
    m_cwd = TRY(Vfs::open_directory(path, m_cwd));
    return 0;
}

SyscallResult Process::sys_close(uint32_t fd) {
    ScopedLock lock(m_lock);
    if (fd >= m_fds.size() || !m_fds[fd]) {
        return Error::BadFd;
    }
    m_fds[fd].clear();
    return 0;
}

SyscallResult Process::sys_connect(const char *path) {
    auto file = TRY(Vfs::open(path, UB_OPEN_MODE_NONE, m_cwd));
    if (!file->is_server_socket()) {
        return Error::Invalid;
    }
    auto client = ustd::make_shared<Socket>(new DoubleBuffer(64_KiB), new DoubleBuffer(64_KiB));
    auto &server = static_cast<ServerSocket &>(*file);
    if (auto rc = server.queue_connection_from(client); rc.is_error()) {
        return rc.error();
    }
    Thread::current().block<ConnectBlocker>(client);
    ScopedLock lock(m_lock);
    uint32_t client_fd = allocate_fd();
    m_fds[client_fd].emplace(client);
    return client_fd;
}

SyscallResult Process::sys_create_pipe(uint32_t *fds) {
    ScopedLock lock(m_lock);
    auto pipe = ustd::make_shared<Pipe>();
    uint32_t read_fd = allocate_fd();
    m_fds[read_fd].emplace(pipe, AttachDirection::Read);
    fds[0] = read_fd;

    uint32_t write_fd = allocate_fd();
    m_fds[write_fd].emplace(pipe, AttachDirection::Write);
    fds[1] = write_fd;
    return 0;
}

SyscallResult Process::sys_create_process(const char *path, const char **argv, ub_fd_pair_t *copy_fds) {
    ScopedLock lock(m_lock);
    auto new_thread = Thread::create_user(ThreadPriority::Normal);
    auto &new_process = new_thread->process();
    new_process.m_cwd = m_cwd;
    new_process.m_fds.ensure_size(m_fds.size());
    for (uint32_t i = 0; i < m_fds.size(); i++) {
        if (m_fds[i]) {
            new_process.m_fds[i].emplace(*m_fds[i]);
        }
    }
    lock.unlock();

    // TODO: Proper validation.
    ASSERT(argv != nullptr);
    ASSERT(copy_fds != nullptr);

    ustd::Vector<ustd::String> args;
    for (size_t i = 0;; i++) {
        if (argv[i] == nullptr) {
            break;
        }
        args.push(argv[i]);
    }
    for (size_t i = 0;; i++) {
        if (copy_fds[i].parent == 0 && copy_fds[i].child == 0) {
            break;
        }
        auto &fd_pair = copy_fds[i];
        new_process.m_fds.ensure_size(fd_pair.child + 1);
        new_process.m_fds[fd_pair.child].emplace(*m_fds[fd_pair.parent]);
    }

    ustd::String copied_path(path);
    if (auto rc = new_thread->exec(copied_path, args); rc.is_error()) {
        return rc.error();
    }
    Scheduler::insert_thread(ustd::move(new_thread));
    return new_process.pid();
}

SyscallResult Process::sys_create_server_socket(uint32_t backlog_limit) {
    // TODO: Upper limit for backlog_limit.
    auto server = ustd::make_shared<ServerSocket>(backlog_limit);
    ScopedLock lock(m_lock);
    uint32_t fd = allocate_fd();
    m_fds[fd].emplace(server);
    return fd;
}

SyscallResult Process::sys_debug_line(const char *line) {
    dmesg("[#{}]: {}", m_pid, line);
    return 0;
}

SyscallResult Process::sys_dup_fd(uint32_t src, uint32_t dst) {
    // TODO: Which check should happen first?
    ScopedLock lock(m_lock);
    if (src >= m_fds.size() || !m_fds[src]) {
        return Error::BadFd;
    }
    if (src == dst) {
        return 0;
    }
    m_fds.ensure_size(dst + 1);
    m_fds[dst].emplace(*m_fds[src]);
    return 0;
}

SyscallResult Process::sys_exit(size_t code) {
    if (code != 0) {
        dmesg("[#{}]: sys_exit called with non-zero code {}", m_pid, code);
    }
    Scheduler::yield_and_kill();
    return 0;
}

SyscallResult Process::sys_getcwd(char *path) {
    ScopedLock lock(m_lock);
    ustd::Vector<Inode *> inodes;
    for (auto *inode = m_cwd; inode != Vfs::root_inode(); inode = inode->parent()) {
        inodes.push(inode);
    }
    // TODO: Would be nice to have some kind of ustd::reverse_iterator(inodes) function.
    ustd::Vector<char> path_characters;
    for (uint32_t i = inodes.size(); i-- != 0;) {
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
    __builtin_memcpy(path, path_characters.data(), path_characters.size());
    return 0;
}

SyscallResult Process::sys_getpid() {
    return m_pid;
}

SyscallResult Process::sys_gettime() {
    return TimeManager::ns_since_boot();
}

SyscallResult Process::sys_ioctl(uint32_t fd, ub_ioctl_request_t request, void *arg) {
    ScopedLock lock(m_lock);
    if (fd >= m_fds.size() || !m_fds[fd]) {
        return Error::BadFd;
    }
    return m_fds[fd]->ioctl(request, arg);
}

SyscallResult Process::sys_mkdir(const char *path) {
    ScopedLock lock(m_lock);
    return TRY(Vfs::mkdir(path, m_cwd));
}

SyscallResult Process::sys_mmap(uint32_t fd) {
    ScopedLock lock(m_lock);
    if (fd >= m_fds.size() || !m_fds[fd]) {
        return Error::BadFd;
    }
    return m_fds[fd]->mmap(*m_address_space);
}

SyscallResult Process::sys_mount(const char *target, const char *fs_type) {
    ScopedLock lock(m_lock);
    ustd::UniquePtr<FileSystem> fs;
    if (ustd::StringView(fs_type) == "ram") {
        fs = ustd::make_unique<RamFs>();
    } else {
        return Error::Invalid;
    }
    return TRY(Vfs::mount(target, ustd::move(fs)));
}

SyscallResult Process::sys_open(const char *path, ub_open_mode_t mode) {
    // Try to open the file first so that we don't leak a file descriptor in the event of failure.
    // TODO: Don't assume Read!
    auto file = TRY(Vfs::open(path, mode, m_cwd));
    ScopedLock lock(m_lock);
    uint32_t fd = allocate_fd();
    m_fds[fd].emplace(ustd::move(file), file->is_pipe() ? AttachDirection::Read : AttachDirection::ReadWrite);
    return fd;
}

SyscallResult Process::sys_poll(ub_poll_fd_t *fds, size_t count, ssize_t timeout) {
    ustd::LargeVector<ub_poll_fd_t> poll_fds(count);
    __builtin_memcpy(poll_fds.data(), fds, count * sizeof(ub_poll_fd_t));
    Thread::current().block<PollBlocker>(poll_fds, m_lock, *this, timeout);

    ScopedLock lock(m_lock);
    for (auto &poll_fd : poll_fds) {
        // TODO: Bounds checking.
        auto &handle = m_fds[poll_fd.fd];
        poll_fd.revents = static_cast<ub_poll_events_t>(0);
        if ((poll_fd.events & UB_POLL_EVENT_READ) == UB_POLL_EVENT_READ && !handle->read_would_block()) {
            poll_fd.revents |= UB_POLL_EVENT_READ;
        }
        if ((poll_fd.events & UB_POLL_EVENT_WRITE) == UB_POLL_EVENT_WRITE && !handle->write_would_block()) {
            poll_fd.revents |= UB_POLL_EVENT_WRITE;
        }
    }
    __builtin_memcpy(fds, poll_fds.data(), count * sizeof(ub_poll_fd_t));
    return 0;
}

SyscallResult Process::sys_read(uint32_t fd, void *data, size_t size) {
    ScopedLock lock(m_lock);
    if (fd >= m_fds.size() || !m_fds[fd]) {
        return Error::BadFd;
    }
    auto &handle = m_fds[fd];
    if (!handle->valid()) {
        handle.clear();
        return Error::BrokenHandle;
    }
    if (handle->read_would_block()) {
        lock.unlock();
        Thread::current().block<ReadBlocker>(handle->file(), handle->offset());
        lock.relock(m_lock);
    }
    return TRY(m_fds[fd]->read(data, size));
}

SyscallResult Process::sys_read_directory(const char *path, uint8_t *data) {
    ScopedLock lock(m_lock);
    auto *directory = TRY(Vfs::open_directory(path, m_cwd));
    if (data == nullptr) {
        size_t byte_count = 0;
        for (size_t i = 0; i < directory->size(); i++) {
            auto *child = TRY(directory->child(i));
            byte_count += child->name().length() + 1;
        }
        return byte_count;
    }
    size_t byte_offset = 0;
    for (size_t i = 0; i < directory->size(); i++) {
        auto *child = TRY(directory->child(i));
        __builtin_memcpy(data + byte_offset, child->name().data(), child->name().length());
        data[byte_offset + child->name().length()] = '\0';
        byte_offset += child->name().length() + 1;
    }
    return 0;
}

SyscallResult Process::sys_seek(uint32_t fd, size_t offset, ub_seek_mode_t mode) {
    ScopedLock lock(m_lock);
    if (fd >= m_fds.size() || !m_fds[fd]) {
        return Error::BadFd;
    }
    if (!m_fds[fd]->valid()) {
        m_fds[fd].clear();
        return Error::BrokenHandle;
    }
    return m_fds[fd]->seek(offset, mode);
}

SyscallResult Process::sys_size(uint32_t fd) {
    ScopedLock lock(m_lock);
    if (fd >= m_fds.size() || !m_fds[fd]) {
        return Error::BadFd;
    }
    if (!m_fds[fd]->valid()) {
        m_fds[fd].clear();
        return Error::BrokenHandle;
    }
    auto &file = m_fds[fd]->file();
    if (!file.is_inode_file()) {
        return Error::Invalid;
    }
    return static_cast<InodeFile &>(file).inode()->size();
}

SyscallResult Process::sys_virt_to_phys(uintptr_t virt) {
    return TRY(m_address_space->virt_to_phys(virt));
}

SyscallResult Process::sys_wait_pid(size_t pid) {
    Thread::current().block<WaitBlocker>(pid);
    return 0;
}

SyscallResult Process::sys_write(uint32_t fd, void *data, size_t size) {
    ScopedLock lock(m_lock);
    if (fd >= m_fds.size() || !m_fds[fd]) {
        return Error::BadFd;
    }
    auto &handle = m_fds[fd];
    if (!handle->valid()) {
        handle.clear();
        return Error::BrokenHandle;
    }
    if (handle->write_would_block()) {
        lock.unlock();
        Thread::current().block<WriteBlocker>(handle->file(), handle->offset());
        lock.relock(m_lock);
    }
    return TRY(m_fds[fd]->write(data, size));
}

} // namespace kernel
