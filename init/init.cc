#include <kernel/Syscall.hh>

[[noreturn]] extern "C" void main() {
    Syscall::invoke(Syscall::print, "Hello, world!");
    while (true) {
    }
}
