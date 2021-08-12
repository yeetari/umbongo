#pragma once

#include <signal.h>
#include <stdint.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

struct __jmp_buf {
    uint64_t regs[8];
    bool did_save_signal_mask;
    sigset_t saved_signal_mask;
};

typedef struct __jmp_buf jmp_buf[1]; // NOLINT
typedef struct __jmp_buf sigjmp_buf[1]; // NOLINT

int setjmp(jmp_buf);
void longjmp(jmp_buf, int);

int sigsetjmp(sigjmp_buf, int);
void siglongjmp(sigjmp_buf, int);

__END_DECLS
