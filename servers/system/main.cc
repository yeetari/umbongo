#include <core/Process.hh>
#include <kernel/Syscall.hh>
#include <kernel/SyscallTypes.hh>
#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/Types.hh>

usize main(usize, const char **) {
    // Mount devfs.
    Syscall::invoke(Syscall::mkdir, "/dev");
    Syscall::invoke(Syscall::mount, "/dev", "dev");

    Array<uint32, 2> pipe_fds{};
    Syscall::invoke(Syscall::create_pipe, pipe_fds.data());

    Syscall::invoke(Syscall::dup_fd, pipe_fds[0], 0);
    if (pipe_fds[0] != 0) {
        Syscall::invoke(Syscall::close, pipe_fds[0]);
    }

    if (core::create_process("/bin/console-server") < 0) {
        ENSURE_NOT_REACHED("Failed to start console server");
    }

    Syscall::invoke(Syscall::dup_fd, pipe_fds[1], 1);
    if (pipe_fds[1] != 1) {
        Syscall::invoke(Syscall::close, pipe_fds[1]);
    }

    ssize kb_fd = -1;
    while (kb_fd < 0) {
        kb_fd = Syscall::invoke(Syscall::open, "/dev/kb", OpenMode::None);
    }
    Syscall::invoke(Syscall::dup_fd, kb_fd, 0);
    if (kb_fd != 0) {
        Syscall::invoke(Syscall::close, kb_fd);
    }

    if (core::create_process("/bin/shell") < 0) {
        ENSURE_NOT_REACHED("Failed to start shell");
    }
    return 0;
}
