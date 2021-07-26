#include <core/File.hh>
#include <kernel/Syscall.hh>
#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/Log.hh>
#include <ustd/Span.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>

int main() {
    Array<uint32, 2> pipe_fds{};
    Syscall::invoke(Syscall::create_pipe, pipe_fds.data());

    Syscall::invoke(Syscall::dup_fd, pipe_fds[0], 0);
    if (pipe_fds[0] != 0) {
        Syscall::invoke(Syscall::close, pipe_fds[0]);
    }

    if (Syscall::invoke(Syscall::create_process, "/console-server") < 0) {
        ENSURE_NOT_REACHED();
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
    return static_cast<int>(pid);
}
