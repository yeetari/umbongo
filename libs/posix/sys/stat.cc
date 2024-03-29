#include <sys/stat.h>

#include <sys/cdefs.h>
#include <sys/types.h>

#include <system/syscall.hh>
#include <ustd/assert.hh>
#include <ustd/try.hh>
#include <ustd/types.hh>

__BEGIN_DECLS

int chmod(const char *, mode_t) {
    ENSURE_NOT_REACHED();
}

int fchmod(int, mode_t) {
    ENSURE_NOT_REACHED();
}

int mkdir(const char *, mode_t) {
    ENSURE_NOT_REACHED();
}

int stat(const char *, struct stat *st) {
    // TODO
    __builtin_memset(st, 0, sizeof(struct stat));
    return 0;
}

int fstat(int fd, struct stat *st) {
    __builtin_memset(st, 0, sizeof(struct stat));
    ssize_t size = EXPECT(system::syscall<ssize_t>(UB_SYS_size, fd));
    ASSERT(size >= 0);
    st->st_mode = S_IFREG | S_IRWXU;
    st->st_size = size;
    return 0;
}

int lstat(const char *path, struct stat *st) {
    return stat(path, st);
}

mode_t umask(mode_t) {
    ENSURE_NOT_REACHED();
}

__END_DECLS
