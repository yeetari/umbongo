#include <core/process.hh>

#include <system/syscall.hh>
#include <ustd/array.hh>
#include <ustd/assert.hh>
#include <ustd/result.hh>
#include <ustd/string.hh>
#include <ustd/try.hh>
#include <ustd/types.hh>
#include <ustd/vector.hh>

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
