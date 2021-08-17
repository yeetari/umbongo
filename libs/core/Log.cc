#include <kernel/Syscall.hh>

void dbg_put_char(char ch) {
    Syscall::invoke(Syscall::putchar, ch);
}

void log_put_char(char ch) {
    Syscall::invoke(Syscall::write, 1, &ch, 1);
}
