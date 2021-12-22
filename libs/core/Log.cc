#include <kernel/Syscall.hh>

void dbg_put_char(char ch) {
    static_cast<void>(Syscall::invoke(Syscall::putchar, ch));
}

void put_char(char ch) {
    static_cast<void>(Syscall::invoke(Syscall::write, 1, &ch, 1));
}
