#include <core/Pipe.hh>

#include <core/File.hh>
#include <kernel/SysError.hh>
#include <kernel/Syscall.hh>
#include <ustd/Array.hh>
#include <ustd/Result.hh>
#include <ustd/Types.hh>

namespace core {

ustd::Result<Pipe, SysError> create_pipe() {
    ustd::Array<uint32, 2> pipe_fds{};
    TRY(Syscall::invoke(Syscall::create_pipe, pipe_fds.data()));
    return Pipe(File(pipe_fds[0]), File(pipe_fds[1]));
}

ustd::Result<void, SysError> Pipe::rebind_read(uint32 fd) {
    TRY(m_read_end.rebind(fd));
    return {};
}

ustd::Result<void, SysError> Pipe::rebind_write(uint32 fd) {
    TRY(m_write_end.rebind(fd));
    return {};
}

} // namespace core
