#include <stdlib.h>

#include <ctype.h>
#include <stddef.h>
#include <sys/cdefs.h>

#include <kernel/Syscall.hh>
#include <ustd/Assert.hh>
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

unsigned long strtoul(const char *, char **, int) {
    ENSURE_NOT_REACHED();
}

unsigned long long strtoull(const char *, char **, int) {
    ENSURE_NOT_REACHED();
}

char *getcwd(char *, size_t) {
    ENSURE_NOT_REACHED();
}

char *getenv(const char *) {
    ENSURE_NOT_REACHED();
}

__END_DECLS
