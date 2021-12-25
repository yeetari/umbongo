#include <core/Process.hh>

#include <core/Error.hh>
#include <core/Syscall.hh>
#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/Result.hh>
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

namespace core {

ustd::Result<usize, SysError> create_process(const char *path) {
    ustd::Array<const char *, 2> argv{path, nullptr};
    ustd::Array<kernel::FdPair, 1> copy_fds{{{0, 0}}};
    return TRY(syscall(Syscall::create_process, path, argv.data(), copy_fds.data()));
}

ustd::Result<usize, SysError> create_process(const char *path, ustd::Vector<const char *> argv) {
    argv.push(nullptr);
    ustd::Array<kernel::FdPair, 1> copy_fds{{{0, 0}}};
    return TRY(syscall(Syscall::create_process, path, argv.data(), copy_fds.data()));
}

ustd::Result<usize, SysError> create_process(const char *path, ustd::Vector<const char *> argv,
                                             ustd::Vector<kernel::FdPair> copy_fds) {
    argv.push(nullptr);
    copy_fds.push({0, 0});
    return TRY(syscall(Syscall::create_process, path, argv.data(), copy_fds.data()));
}

[[noreturn]] void exit(usize code) {
    EXPECT(syscall(Syscall::exit, code));
    ENSURE_NOT_REACHED();
}


} // namespace core
