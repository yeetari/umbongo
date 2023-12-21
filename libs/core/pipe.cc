#include <core/pipe.hh>

#include <core/error.hh>
#include <core/file.hh>
#include <system/syscall.hh>
#include <ustd/array.hh>
#include <ustd/result.hh>
#include <ustd/try.hh>
#include <ustd/types.hh>

namespace core {

ustd::Result<Pipe, ub_error_t> create_pipe() {
    ustd::Array<uint32_t, 2> pipe_fds{};
    TRY(system::syscall(UB_SYS_create_pipe, pipe_fds.data()));
    return Pipe(File(pipe_fds[0]), File(pipe_fds[1]));
}

ustd::Result<void, ub_error_t> Pipe::rebind_read(uint32_t fd) {
    TRY(m_read_end.rebind(fd));
    return {};
}

ustd::Result<void, ub_error_t> Pipe::rebind_write(uint32_t fd) {
    TRY(m_write_end.rebind(fd));
    return {};
}

} // namespace core
