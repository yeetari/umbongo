#include <kernel/Syscall.hh>

extern "C" void main() {
    uint64 pid = Syscall::invoke(Syscall::getpid);
    Syscall::invoke(Syscall::print, "Hello world!");
    Syscall::invoke(Syscall::exit, pid);
}
