#include <core/print.hh>
#include <ustd/types.hh>

size_t main(size_t argc, const char **argv) {
    while (true) {
        for (size_t i = 1; i < argc; i++) {
            core::print("{} ", argv[i]);
        }
        if (argc == 1) {
            core::put_char('y');
        }
        core::put_char('\n');
    }
}
