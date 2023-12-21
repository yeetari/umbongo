#include <time.h>

#include <stddef.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include <ustd/assert.hh>

__BEGIN_DECLS

char *ctime(const time_t *) {
    ENSURE_NOT_REACHED("TODO");
}

time_t time(time_t *) {
    ENSURE_NOT_REACHED("TODO");
}

int gettimeofday(struct timeval *, void *) {
    ENSURE_NOT_REACHED();
}

struct tm *gmtime(const time_t *) {
    ENSURE_NOT_REACHED();
}

struct tm *localtime(const time_t *) {
    ENSURE_NOT_REACHED();
}

size_t strftime(char *, size_t, const char *, const struct tm *) {
    ENSURE_NOT_REACHED();
}

__END_DECLS
