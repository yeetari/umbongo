#include <core/Pipe.hh>

#include <core/Error.hh>
#include <core/File.hh>
#include <core/Syscall.hh>
#include <ustd/Array.hh>
#include <ustd/Result.hh>
#include <ustd/Try.hh>
#include <ustd/Types.hh>

namespace core {

ustd::Result<Pipe, SysError> create_pipe() {
    ustd::Array<uint32_t, 2> pipe_fds{};
    TRY(syscall(Syscall::create_pipe, pipe_fds.data()));
    return Pipe(File(pipe_fds[0]), File(pipe_fds[1]));
}

ustd::Result<void, SysError> Pipe::rebind_read(uint32_t fd) {
    TRY(m_read_end.rebind(fd));
    return {};
}

ustd::Result<void, SysError> Pipe::rebind_write(uint32_t fd) {
    TRY(m_write_end.rebind(fd));
    return {};
}

} // namespace core
