#include <core/Process.hh>

#include <system/Syscall.hh>
#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/Result.hh>
#include <ustd/String.hh>
#include <ustd/Try.hh>
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

namespace core {

ustd::Result<void, ub_error_t> chdir(const char *path) {
    TRY(system::syscall(UB_SYS_chdir, path));
    return {};
}

ustd::Result<size_t, ub_error_t> create_process(const char *path) {
    ustd::Array<const char *, 2> argv{path, nullptr};
    ustd::Array<ub_fd_pair_t, 1> copy_fds{{{0, 0}}};
    return TRY(system::syscall(UB_SYS_create_process, path, argv.data(), copy_fds.data()));
}

ustd::Result<size_t, ub_error_t> create_process(const char *path, ustd::Vector<const char *> argv) {
    argv.push(nullptr);
    ustd::Array<ub_fd_pair_t, 1> copy_fds{{{0, 0}}};
    return TRY(system::syscall(UB_SYS_create_process, path, argv.data(), copy_fds.data()));
}

ustd::Result<size_t, ub_error_t> create_process(const char *path, ustd::Vector<const char *> argv,
                                                ustd::Vector<ub_fd_pair_t> copy_fds) {
    argv.push(nullptr);
    copy_fds.push({0, 0});
    return TRY(system::syscall(UB_SYS_create_process, path, argv.data(), copy_fds.data()));
}

[[noreturn]] void exit(size_t code) {
    EXPECT(system::syscall(UB_SYS_exit, code));
    ENSURE_NOT_REACHED();
}

ustd::String cwd() {
    auto cwd_length = EXPECT(system::syscall(UB_SYS_getcwd, nullptr));
    ustd::String cwd(cwd_length);
    EXPECT(system::syscall(UB_SYS_getcwd, cwd.data()));
    return cwd;
}

size_t pid() {
    return EXPECT(system::syscall(UB_SYS_getpid));
}

ustd::Result<void, ub_error_t> wait_pid(size_t pid) {
    TRY(system::syscall(UB_SYS_wait_pid, pid));
    return {};
}

} // namespace core
