#include <sys/wait.h>

#include <sys/cdefs.h>

#include <ustd/Assert.hh>

__BEGIN_DECLS

pid_t wait(int *) {
    ENSURE_NOT_REACHED();
}

__END_DECLS
