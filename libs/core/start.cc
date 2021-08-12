#include <kernel/Syscall.hh>
#include <ustd/Types.hh>

usize main(usize argc, const char **argv);

extern "C" void _start(usize argc, const char **argv) {
    asm volatile("andq $~15, %rsp");
    Syscall::invoke(Syscall::exit, main(argc, argv));
}
