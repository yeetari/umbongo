#include <kernel/Syscall.hh>

void put_char(char ch) {
    Syscall::invoke(Syscall::putchar, ch);
}
