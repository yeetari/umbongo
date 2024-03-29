#include <fcntl.h>

#include <bits/error.hh>
#include <errno.h>
#include <stdarg.h>
#include <sys/cdefs.h>

#include <log/log.hh>
#include <system/syscall.hh>
#include <ustd/result.hh>
#include <ustd/types.hh>

__BEGIN_DECLS

int fcntl(int fd, int cmd, ...) {
    va_list ap;
    va_start(ap, cmd);
    auto arg = va_arg(ap, uint32_t);
    va_end(ap);
    if (cmd == F_GETFD || cmd == F_SETFD) {
        return 0;
    }
    log::error("Unknown fcntl({}, {}, {})", fd, cmd, arg);
    errno = EINVAL;
    return -1;
}

int open(const char *path, int oflag, ...) {
    auto flags = static_cast<uint32_t>(oflag);
    flags &= ~(O_RDONLY | O_WRONLY);

    auto mode = UB_OPEN_MODE_NONE;
    if ((flags & O_CREAT) == O_CREAT) {
        mode |= UB_OPEN_MODE_CREATE;
    }
    if ((flags & O_TRUNC) == O_TRUNC) {
        mode |= UB_OPEN_MODE_TRUNCATE;
    }
    flags &= ~(O_CREAT | O_TRUNC);
    if (flags != 0) {
        log::error("Unknown open flag bitset {:h}", flags);
        errno = EINVAL;
        return -1;
    }

    auto result = system::syscall(UB_SYS_open, path, mode);
    if (result.is_error()) {
        errno = posix::to_errno(result.error());
        return -1;
    }
    return static_cast<int>(result.value());
}

__END_DECLS
