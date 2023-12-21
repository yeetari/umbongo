#include <sys/cdefs.h>

#include <log/log.hh>
#include <ustd/string.hh>
#include <ustd/string_builder.hh>
#include <ustd/types.hh>

__BEGIN_DECLS

int errno;
char **environ;

void __stdio_init();
int main(int argc, char **argv);

__END_DECLS

size_t main(size_t argc, const char **argv) {
    log::initialise(ustd::format("posix-{}", argv[0]));
    __stdio_init();
    return static_cast<size_t>(main(static_cast<int>(argc), const_cast<char **>(argv)));
}
