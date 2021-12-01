#include <core/Pipe.hh>

#include <core/File.hh>
#include <kernel/Syscall.hh>
#include <ustd/Array.hh>
#include <ustd/Optional.hh>
#include <ustd/Types.hh>

namespace core {

ustd::Optional<Pipe> create_pipe() {
    ustd::Array<uint32, 2> pipe_fds{};
    if (Syscall::invoke(Syscall::create_pipe, pipe_fds.data()) < 0) {
        return {};
    }
    return {{File(pipe_fds[0]), File(pipe_fds[1])}};
}

ssize Pipe::rebind_read(uint32 fd) {
    return m_read_end.rebind(fd);
}

ssize Pipe::rebind_write(uint32 fd) {
    return m_write_end.rebind(fd);
}

} // namespace core
