#include <fcntl.h>

#include <bits/error.hh>
#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <sys/cdefs.h>

#include <kernel/Syscall.hh>
#include <kernel/SyscallTypes.hh>
#include <ustd/Assert.hh>
#include <ustd/Log.hh>

__BEGIN_DECLS

int fcntl(int fd, int cmd, ...) {
    va_list ap;
    va_start(ap, cmd);
    auto arg = va_arg(ap, uint32_t);
    va_end(ap);
    if (cmd == F_GETFD || cmd == F_SETFD) {
        return 0;
    }
    dbgln("Unknown fcntl({}, {}, {})", fd, cmd, arg);
    ENSURE_NOT_REACHED("Unknown fcntl");
}

int open(const char *path, int oflag, ...) {
    auto flags = static_cast<uint32_t>(oflag);
    flags &= ~(O_RDONLY | O_WRONLY);

    auto mode = OpenMode::None;
    if ((flags & O_CREAT) == O_CREAT) {
        mode |= OpenMode::Create;
    }
    if ((flags & O_TRUNC) == O_TRUNC) {
        mode |= OpenMode::Truncate;
    }
    flags &= ~(O_CREAT | O_TRUNC);
    if (flags != 0) {
        dbgln("Unknown open flag bitset {:h}", flags);
        ENSURE_NOT_REACHED();
    }

    auto rc = Syscall::invoke(Syscall::open, path, mode);
    if (rc < 0) {
        errno = posix::to_errno(rc);
        return -1;
    }
    return static_cast<int>(rc);
}

__END_DECLS
