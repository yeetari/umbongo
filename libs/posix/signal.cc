#include <signal.h>

#include <sys/cdefs.h>
#include <sys/types.h>

#include <ustd/assert.hh>

__BEGIN_DECLS

int kill(pid_t, int) {
    ENSURE_NOT_REACHED();
}

sighandler_t signal(int, sighandler_t) {
    ENSURE_NOT_REACHED();
}

__END_DECLS
