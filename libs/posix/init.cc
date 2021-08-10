#include <sys/cdefs.h>

#include <ustd/Types.hh>

__BEGIN_DECLS

int errno;

void __stdio_init();
int main(int argc, char **argv);

__END_DECLS

usize main(usize argc, const char **argv) {
    __stdio_init();
    return static_cast<usize>(main(static_cast<int>(argc), const_cast<char **>(argv)));
}
