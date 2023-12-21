#include <setjmp.h>

#include <sys/cdefs.h>

#include <ustd/assert.hh>

__BEGIN_DECLS

int sigsetjmp(sigjmp_buf, int) {
    ENSURE_NOT_REACHED();
}

void siglongjmp(sigjmp_buf, int) {
    ENSURE_NOT_REACHED();
}

__END_DECLS
