#include <locale.h>

#include <sys/cdefs.h>

__BEGIN_DECLS

char *setlocale(int, const char *) {
    return const_cast<char *>("C");
}

__END_DECLS
