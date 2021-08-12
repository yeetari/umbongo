#include <fcntl.h>

#include <stdarg.h>
#include <stdint.h>
#include <sys/cdefs.h>

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

int open(const char *, int, ...) {
    ENSURE_NOT_REACHED();
}

__END_DECLS
