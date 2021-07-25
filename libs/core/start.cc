#include <kernel/Syscall.hh>

int main();

extern "C" void _start() {
    Syscall::invoke(Syscall::exit, main());
}
