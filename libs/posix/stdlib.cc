#include <stdlib.h>

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <sys/cdefs.h>

#include <kernel/Syscall.hh>
#include <ustd/Algorithm.hh>
#include <ustd/Assert.hh>
#include <ustd/Log.hh>
#include <ustd/Result.hh>
#include <ustd/Utility.hh>

namespace {

struct QsortObject {
    char *base;
    size_t size;
};

class QsortContainer {
    char *const m_base;
    const size_t m_element_size;
    const size_t m_size;

public:
    QsortContainer(void *base, size_t element_size, size_t size)
        : m_base(reinterpret_cast<char *>(base)), m_element_size(element_size), m_size(size) {}

    // TODO: Why does this have to return a const QsortObject? Compiler bug?
    // NOLINTNEXTLINE
    const QsortObject operator[](size_t index) const { return {m_base + index * m_element_size, m_element_size}; }
    void *data() const { return m_base; }
    size_t size() const { return m_size; }
    size_t size_bytes() const { return m_size * m_element_size; }
};

} // namespace

namespace ustd {

template <>
void swap(const QsortObject &lhs, const QsortObject &rhs) {
    char *s1 = lhs.base;
    char *s2 = rhs.base;
    for (size_t n = lhs.size; n > 0; n--, s1++, s2++) {
        swap(*s1, *s2);
    }
}

} // namespace ustd

__BEGIN_DECLS

void abort() {
    ENSURE_NOT_REACHED("abort");
}

void exit(int status) {
    EXPECT(Syscall::invoke(Syscall::exit, status));
    ENSURE_NOT_REACHED();
}

void *malloc(size_t size) {
    return operator new(size);
}

void *calloc(size_t count, size_t size) {
    void *ptr = malloc(count * size);
    __builtin_memset(ptr, 0, count * size);
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
        __builtin_memcpy(new_ptr, ptr, size);
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

double strtod(const char *, char **) {
    ENSURE_NOT_REACHED();
}

float strtof(const char *, char **) {
    ENSURE_NOT_REACHED();
}

long strtol(const char *, char **, int) {
    ENSURE_NOT_REACHED();
}

long double strtold(const char *, char **) {
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
    ustd::dbgln("getenv({})", name);
    return nullptr;
}

size_t mbstowcs(uint16_t *, const char *, size_t) {
    ENSURE_NOT_REACHED();
}

char *mktemp(char *) {
    ENSURE_NOT_REACHED();
}

void qsort(void *base, size_t count, size_t size, int (*compare)(const void *, const void *)) {
    QsortContainer container(base, size, count);
    ustd::sort(container, [=](const QsortObject &lhs, const QsortObject &rhs) {
        return compare(lhs.base, rhs.base) > 0;
    });
}

__END_DECLS
