#include <core/Process.hh>

#include <core/Error.hh>
#include <core/Syscall.hh>
#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/Result.hh>
#include <ustd/String.hh>
#include <ustd/Try.hh>
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

namespace core {

ustd::Result<void, SysError> chdir(const char *path) {
    TRY(syscall(Syscall::chdir, path));
    return {};
}

ustd::Result<size_t, SysError> create_process(const char *path) {
    ustd::Array<const char *, 2> argv{path, nullptr};
    ustd::Array<kernel::FdPair, 1> copy_fds{{{0, 0}}};
    return TRY(syscall(Syscall::create_process, path, argv.data(), copy_fds.data()));
}

ustd::Result<size_t, SysError> create_process(const char *path, ustd::Vector<const char *> argv) {
    argv.push(nullptr);
    ustd::Array<kernel::FdPair, 1> copy_fds{{{0, 0}}};
    return TRY(syscall(Syscall::create_process, path, argv.data(), copy_fds.data()));
}

ustd::Result<size_t, SysError> create_process(const char *path, ustd::Vector<const char *> argv,
                                              ustd::Vector<kernel::FdPair> copy_fds) {
    argv.push(nullptr);
    copy_fds.push({0, 0});
    return TRY(syscall(Syscall::create_process, path, argv.data(), copy_fds.data()));
}

[[noreturn]] void exit(size_t code) {
    EXPECT(syscall(Syscall::exit, code));
    ENSURE_NOT_REACHED();
}

ustd::String cwd() {
    auto cwd_length = EXPECT(syscall(Syscall::getcwd, nullptr));
    ustd::String cwd(cwd_length);
    EXPECT(syscall(Syscall::getcwd, cwd.data()));
    return cwd;
}

size_t pid() {
    return EXPECT(syscall(Syscall::getpid));
}

ustd::Result<void, SysError> wait_pid(size_t pid) {
    TRY(syscall(Syscall::wait_pid, pid));
    return {};
}

} // namespace core
