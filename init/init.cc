#include <kernel/Syscall.hh>
#include <ustd/Types.hh>

extern "C" void main() {
    uint64 pid = Syscall::invoke(Syscall::getpid);
    Syscall::invoke(Syscall::print, "Hello world!");
    Syscall::invoke(Syscall::exit, pid);
}
