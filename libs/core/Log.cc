#include <kernel/Syscall.hh>

void dbg_put_char(char ch) {
    Syscall::invoke(Syscall::putchar, ch);
}

// TODO: So many problems with this.
void log_put_char(char ch) {
    while (Syscall::invoke(Syscall::write, 1, &ch, 1) != 1) {
    }
}
