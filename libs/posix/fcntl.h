#pragma once

#include <sys/cdefs.h>

__BEGIN_DECLS

#define F_GETFD 0
#define F_SETFD 1

#define FD_CLOEXEC 0

#define O_RDONLY (1u << 0u)
#define O_WRONLY (1u << 1u)
#define O_EXEC (1u << 2u)
#define O_CREAT (1u << 3u)
#define O_EXCL (1u << 4u)
#define O_NOCTTY (1u << 5u)
#define O_TRUNC (1u << 6u)
#define O_APPEND (1u << 7u)
#define O_NONBLOCK (1u << 8u)
#define O_DIRECTORY (1u << 9u)
#define O_NOFOLLOW (1u << 10u)
#define O_CLOEXEC (1u << 11u)
#define O_DIRECT (1u << 12u)
#define O_RDWR (O_RDONLY | O_WRONLY)
#define O_ACCMODE (O_RDONLY | O_WRONLY)

int fcntl(int, int, ...);
int open(const char *, int, ...);

__END_DECLS
