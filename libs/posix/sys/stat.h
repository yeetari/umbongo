#pragma once

#include <fcntl.h> // IWYU pragma: keep
#include <sys/cdefs.h>
#include <sys/types.h> // IWYU pragma: keep
#include <time.h>

__BEGIN_DECLS

#define S_IFMT 0170000u
#define S_IFDIR 0040000u
#define S_IFCHR 0020000u
#define S_IFBLK 0060000u
#define S_IFREG 0100000u
#define S_IFIFO 0010000u
#define S_IFLNK 0120000u
#define S_IFSOCK 0140000u

#define S_ISUID 04000u
#define S_ISGID 02000u
#define S_ISVTX 01000u
#define S_IRUSR 0400u
#define S_IWUSR 0200u
#define S_IXUSR 0100u
#define S_IREAD S_IRUSR
#define S_IWRITE S_IWUSR
#define S_IEXEC S_IXUSR
#define S_IRGRP 0040u
#define S_IWGRP 0020u
#define S_IXGRP 0010u
#define S_IROTH 0004u
#define S_IWOTH 0002u
#define S_IXOTH 0001u

#define S_IRWXU (S_IRUSR | S_IWUSR | S_IXUSR)

#define S_IRWXG (S_IRWXU >> 3)
#define S_IRWXO (S_IRWXG >> 3)
#define S_ISDIR(m) (((m)&S_IFMT) == S_IFDIR)
#define S_ISCHR(m) (((m)&S_IFMT) == S_IFCHR)
#define S_ISBLK(m) (((m)&S_IFMT) == S_IFBLK)
#define S_ISREG(m) (((m)&S_IFMT) == S_IFREG)
#define S_ISFIFO(m) (((m)&S_IFMT) == S_IFIFO)
#define S_ISLNK(m) (((m)&S_IFMT) == S_IFLNK)
#define S_ISSOCK(m) (((m)&S_IFMT) == S_IFSOCK)

struct stat {
    dev_t st_dev;
    ino_t st_ino;
    mode_t st_mode;
    nlink_t st_nlink;
    uid_t st_uid;
    gid_t st_gid;
    dev_t st_rdev;
    off_t st_size;
    blksize_t st_blksize;
    blkcnt_t st_blocks;
    struct timespec st_atim;
    struct timespec st_mtim;
    struct timespec st_ctim;
};

#define st_atime st_atim.tv_sec
#define st_mtime st_mtim.tv_sec
#define st_ctime st_ctim.tv_sec

int chmod(const char *, mode_t);
int fchmod(int, mode_t);
int mkdir(const char *, mode_t);
int stat(const char *, struct stat *);
int fstat(int, struct stat *);
int lstat(const char *, struct stat *);
mode_t umask(mode_t);

__END_DECLS
