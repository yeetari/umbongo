#include <core/Print.hh>
#include <ustd/Types.hh>

usize main(usize argc, const char **argv) {
    while (true) {
        for (usize i = 1; i < argc; i++) {
            core::print("{} ", argv[i]);
        }
        if (argc == 1) {
            core::put_char('y');
        }
        core::put_char('\n');
    }
}
