#include <core/Syscall.hh>

void dbg_put_char(char ch) {
    static_cast<void>(core::syscall(Syscall::putchar, ch));
}

void put_char(char ch) {
    static_cast<void>(core::syscall(Syscall::write, 1, &ch, 1));
}
