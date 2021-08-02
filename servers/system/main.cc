#include <core/File.hh>
#include <kernel/Syscall.hh>
#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/Log.hh>
#include <ustd/Span.hh>
#include <ustd/StringView.hh>
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

    if (Syscall::invoke(Syscall::create_process, "/console-server") < 0) {
        ENSURE_NOT_REACHED("Failed to start console server");
    }

    Syscall::invoke(Syscall::dup_fd, pipe_fds[1], 1);
    if (pipe_fds[1] != 1) {
        Syscall::invoke(Syscall::close, pipe_fds[1]);
    }

    core::File file("/home/hello");
    ENSURE(file, "Failed to open /home/hello");

    Array<char, 20> data{'\0'};
    file.read(data.span());

    auto pid = Syscall::invoke<uint64>(Syscall::getpid);
    logln("[#{}]: {}", pid, static_cast<const char *>(data.data()));

    core::File keyboard;
    while (true) {
        char ch = '\0';
        auto nread = keyboard.read({&ch, 1});
        if (nread < 0) {
            while (!keyboard.open("/dev/kb")) {
            }
        } else if (nread == 1) {
            log("{:c}", ch);
        }
    }
}
