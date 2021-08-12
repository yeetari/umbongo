#include <utime.h>

#include <sys/cdefs.h>

#include <ustd/Assert.hh>

__BEGIN_DECLS

int utime(const char *, const struct utimbuf *) {
    ENSURE_NOT_REACHED();
}

__END_DECLS
