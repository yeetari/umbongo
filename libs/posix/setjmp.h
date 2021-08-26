#pragma once

#include <stdint.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

// NOLINTNEXTLINE
typedef struct __jmp_buf {
    // rbx, r12, r13, r14, r15, rbp, rsp, rip.
    uint64_t regs[8];
} jmp_buf[1];
typedef jmp_buf sigjmp_buf;

int setjmp(jmp_buf);
__attribute__((noreturn)) void longjmp(jmp_buf, int);

int sigsetjmp(sigjmp_buf, int);
__attribute__((noreturn)) void siglongjmp(sigjmp_buf, int);

__END_DECLS
