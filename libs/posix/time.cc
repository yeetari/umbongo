#include <time.h>

#include <sys/cdefs.h>

#include <ustd/Assert.hh>

__BEGIN_DECLS

time_t time(time_t *) {
    ENSURE_NOT_REACHED("TODO");
}

char *ctime(const time_t *) {
    ENSURE_NOT_REACHED("TODO");
}

__END_DECLS
