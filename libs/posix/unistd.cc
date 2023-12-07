#include <unistd.h>

#include <bits/error.hh>
#include <errno.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include <log/Log.hh>
#include <system/Syscall.hh>
#include <ustd/Assert.hh>
#include <ustd/Result.hh>
#include <ustd/Types.hh>

__BEGIN_DECLS

int access(const char *, int) {
    ENSURE_NOT_REACHED();
}

int close(int fd) {
    if (auto result = system::syscall(UB_SYS_close, fd); result.is_error()) {
        errno = posix::to_errno(result.error());
        return -1;
    }
    return 0;
}

void _exit(int) {
    ENSURE_NOT_REACHED();
}

pid_t getpid(void) {
    ENSURE_NOT_REACHED();
}

int unlink(const char *path) {
    log::warn("TODO: Implement unlink({})", path);
    return 0;
}

int dup(int) {
    ENSURE_NOT_REACHED();
}

int dup2(int, int) {
    ENSURE_NOT_REACHED();
}

int isatty(int) {
    return 0;
}

int pipe(int[2]) {
    ENSURE_NOT_REACHED();
}

pid_t fork(void) {
    ENSURE_NOT_REACHED();
}

int execv(const char *, char *const[]) {
    ENSURE_NOT_REACHED();
}

int execve(const char *, char *const[], char *const[]) {
    ENSURE_NOT_REACHED();
}

int execvp(const char *, char *const[]) {
    ENSURE_NOT_REACHED();
}

unsigned sleep(unsigned) {
    ENSURE_NOT_REACHED();
}

int chdir(const char *) {
    ENSURE_NOT_REACHED();
}

int rmdir(const char *) {
    ENSURE_NOT_REACHED();
}

off_t lseek(int, off_t, int) {
    ENSURE_NOT_REACHED();
}

ssize_t read(int fd, void *data, size_t size) {
    auto result = system::syscall<ssize_t>(UB_SYS_read, fd, data, size);
    if (result.is_error()) {
        errno = posix::to_errno(result.error());
        return -1;
    }
    return result.value();
}

ssize_t write(int, const void *, size_t) {
    ENSURE_NOT_REACHED();
}

__END_DECLS
