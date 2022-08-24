#include <sys/cdefs.h>

#include <ustd/Types.hh>

__BEGIN_DECLS

int errno;
char **environ;

void __stdio_init();
int main(int argc, char **argv);

__END_DECLS

size_t main(size_t argc, const char **argv) {
    __stdio_init();
    return static_cast<size_t>(main(static_cast<int>(argc), const_cast<char **>(argv)));
}
