#include <ustd/Log.hh>
#include <ustd/Types.hh>

usize main(usize argc, const char **argv) {
    while (true) {
        for (usize i = 1; i < argc; i++) {
            printf("{} ", argv[i]);
        }
        if (argc == 1) {
            put_char('y');
        }
        put_char('\n');
    }
}
