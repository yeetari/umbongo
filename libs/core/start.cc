#include <kernel/Syscall.hh>
#include <ustd/Types.hh>

usize main(usize argc, const char **argv);

extern "C" void _start(usize argc, const char **argv) {
    Syscall::invoke(Syscall::exit, main(argc, argv));
}
