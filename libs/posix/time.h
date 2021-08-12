#pragma once

#include <sys/cdefs.h>
#include <sys/types.h> // IWYU pragma: keep

__BEGIN_DECLS

struct tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};

struct timespec {
    time_t tv_sec;
    long tv_nsec;
};

char *ctime(const time_t *);
time_t time(time_t *);

struct tm *gmtime(const time_t *);
struct tm *localtime(const time_t *);
size_t strftime(char *, size_t, const char *, const struct tm *);

__END_DECLS
