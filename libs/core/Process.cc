#include <core/Process.hh>

#include <kernel/Syscall.hh>
#include <kernel/SyscallTypes.hh>
#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

namespace core {

ssize create_process(const char *path) {
    ustd::Array<const char *, 2> argv{path, nullptr};
    ustd::Array<FdPair, 1> copy_fds{{{0, 0}}};
    return Syscall::invoke(Syscall::create_process, path, argv.data(), copy_fds.data());
}

ssize create_process(const char *path, ustd::Vector<const char *> argv) {
    argv.push(nullptr);
    ustd::Array<FdPair, 1> copy_fds{{{0, 0}}};
    return Syscall::invoke(Syscall::create_process, path, argv.data(), copy_fds.data());
}

ssize create_process(const char *path, ustd::Vector<const char *> argv, ustd::Vector<FdPair> copy_fds) {
    argv.push(nullptr);
    copy_fds.push({0, 0});
    return Syscall::invoke(Syscall::create_process, path, argv.data(), copy_fds.data());
}

[[noreturn]] void exit(usize code) {
    Syscall::invoke(Syscall::exit, code);
    ENSURE_NOT_REACHED();
}

} // namespace core
