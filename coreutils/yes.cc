#include <ustd/Log.hh>
#include <ustd/Types.hh>

usize main(usize argc, const char **argv) {
    while (true) {
        for (usize i = 1; i < argc; i++) {
            log("{} ", argv[i]);
        }
        if (argc == 1) {
            log("y");
        }
        log_put_char('\n');
    }
}
