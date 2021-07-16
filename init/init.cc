#include <kernel/Syscall.hh>
#include <ustd/Log.hh>
#include <ustd/Types.hh>

void put_char(char ch) {
    Syscall::invoke(Syscall::putchar, ch);
}

extern "C" void main() {
    uint64 pid = Syscall::invoke(Syscall::getpid);
    logln("Hello, world!");
    Syscall::invoke(Syscall::exit, pid);
}
