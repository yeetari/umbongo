#include <setjmp.h>

#include <sys/cdefs.h>

#include <ustd/Assert.hh>

__BEGIN_DECLS

int setjmp(jmp_buf) {
    ENSURE_NOT_REACHED();
}

void longjmp(jmp_buf, int) {
    ENSURE_NOT_REACHED();
}

__END_DECLS
