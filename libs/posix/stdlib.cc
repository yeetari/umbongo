#include <stdlib.h>

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <sys/cdefs.h>

#include <kernel/Syscall.hh>
#include <ustd/Assert.hh>
#include <ustd/Log.hh>
#include <ustd/Memory.hh>

__BEGIN_DECLS

void abort() {
    ENSURE_NOT_REACHED("abort");
}

void exit(int status) {
    Syscall::invoke(Syscall::exit, status);
    ENSURE_NOT_REACHED();
}

void *malloc(size_t size) {
    return operator new(size);
}

void *calloc(size_t count, size_t size) {
    void *ptr = malloc(count * size);
    memset(ptr, 0, count * size);
    return ptr;
}

void *realloc(void *ptr, size_t size) {
    if (ptr == nullptr) {
        return malloc(size);
    }
    if (size == 0) {
        free(ptr);
        return nullptr;
    }
    auto *new_ptr = malloc(size);
    if (new_ptr != nullptr) {
        // TODO: Not right at all. We are copying random memory into the new buffer when size is greater than the old
        //       size.
        memcpy(new_ptr, ptr, size);
        free(ptr);
    }
    return new_ptr;
}

void free(void *ptr) {
    operator delete(ptr);
}

int atoi(const char *str) {
    while (isspace(*str)) {
        str++;
    }
    bool neg = *str == '-';
    if (neg || *str == '+') {
        str++;
    }
    int ret = 0;
    while (isdigit(*str)) {
        ret = ret * 10 - (*str++ - '0');
    }
    return neg ? ret : -ret;
}

long atol(const char *) {
    ENSURE_NOT_REACHED();
}

double atof(const char *) {
    ENSURE_NOT_REACHED();
}

unsigned long strtoul(const char *str, char **end, int base) {
    if (end != nullptr) {
        *end = const_cast<char *>(str);
    }
    while (isspace(*str)) {
        str++;
    }
    bool neg = *str == '-';
    if (neg || *str == '+') {
        str++;
    }
    if (base == 0 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        base = 16;
    }
    if (base == 0) {
        base = *str == '0' ? 8 : 10;
    }
    if (base == 16) {
        str += 2;
    }
    const unsigned long cutoff = ULONG_MAX / static_cast<unsigned long>(base);
    const unsigned long cutlim = ULONG_MAX / static_cast<unsigned long>(base);
    unsigned long ret = 0;
    for (char ch = '\0'; (ch = *str) != '\0'; str++) {
        if (isdigit(ch)) {
            ch -= '0';
        } else if (isalpha(ch)) {
            ch -= isupper(ch) ? 'A' - 10 : 'a' - 10;
        } else {
            break;
        }
        if (ch >= base) {
            break;
        }
        if (ret > cutoff || (ret == cutoff && static_cast<unsigned long>(ch) > cutlim)) {
            errno = ERANGE;
            return ULONG_MAX;
        }
        ret *= static_cast<unsigned long>(base);
        ret += static_cast<unsigned long>(ch);
        if (end != nullptr) {
            *end = const_cast<char *>(str - 1);
        }
    }
    return neg ? -ret : ret;
}

unsigned long long strtoull(const char *, char **, int) {
    ENSURE_NOT_REACHED();
}

char *getcwd(char *, size_t) {
    ENSURE_NOT_REACHED();
}

char *getenv(const char *name) {
    dbgln("getenv({})", name);
    return nullptr;
}

size_t mbstowcs(uint16_t *, const char *, size_t) {
    ENSURE_NOT_REACHED();
}

char *mktemp(char *) {
    ENSURE_NOT_REACHED();
}

void qsort(void *base, size_t count, size_t size, int (*compare)(const void *, const void *)) {
    if (count == 0) {
        return;
    }
    size_t gap = count;
    bool swapped = false;
    do {
        gap = (gap * 10) / 13;
        if (gap == 9 || gap == 10) {
            gap = 11;
        }
        if (gap < 1) {
            gap = 1;
        }
        swapped = false;
        char *p1 = static_cast<char *>(base);
        for (size_t i = 0; i < count - gap; i++, p1 += size) {
            size_t j = i + gap;
            char *p2 = static_cast<char *>(base) + j * size;
            if (compare(p1, p2) > 0) {
                char *s1 = p1;
                char *s2 = p2;
                for (size_t n = size; n > 0; n--, s1++, s2++) {
                    char tmp = *s1;
                    *s1 = *s2;
                    *s2 = tmp;
                }
                swapped = true;
            }
        }
    } while (gap > 1 || swapped);
}

__END_DECLS
