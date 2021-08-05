#include <core/Process.hh>

#include <kernel/Syscall.hh>
#include <kernel/SyscallTypes.hh>
#include <ustd/Array.hh>
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

namespace core {

ssize create_process(const char *path) {
    Array<const char *, 2> argv{path, nullptr};
    Array<FdPair, 1> copy_fds{{{0, 0}}};
    return Syscall::invoke(Syscall::create_process, path, argv.data(), copy_fds.data());
}

ssize create_process(const char *path, Vector<const char *> argv) {
    argv.push(nullptr);
    Array<FdPair, 1> copy_fds{{{0, 0}}};
    return Syscall::invoke(Syscall::create_process, path, argv.data(), copy_fds.data());
}

ssize create_process(const char *path, Vector<const char *> argv, Vector<FdPair> copy_fds) {
    argv.push(nullptr);
    copy_fds.push({0, 0});
    return Syscall::invoke(Syscall::create_process, path, argv.data(), copy_fds.data());
}

} // namespace core
