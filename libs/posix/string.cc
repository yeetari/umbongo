#include <string.h>

#include <stdint.h>
#include <sys/cdefs.h>

#include <ustd/Assert.hh>

__BEGIN_DECLS

void *memmove(void *dst, const void *src, size_t size) {
    if (dst < src) {
        return memcpy(dst, src, size);
    }
    auto *d = reinterpret_cast<uint8_t *>(dst) + size;
    const auto *s = reinterpret_cast<const uint8_t *>(src) + size;
    for (size_t i = 0; i < size; i++) {
        *--d = *--s;
    }
    return dst;
}

char *strcat(char *dst, const char *src) {
    strcpy(dst + strlen(dst), src);
    return dst;
}

int strcoll(const char *, const char *) {
    ENSURE_NOT_REACHED();
}

char *strcpy(char *dst, const char *src) {
    for (; (*dst = *src) != '\0'; dst++, src++) {
    }
    return dst;
}

char *strncat(char *dst, const char *src, size_t n) {
    char *ret = dst;
    dst += strlen(dst);
    for (; n != 0 && *src != '\0'; n--) {
        *dst++ = *src++;
    }
    *dst++ = '\0';
    return ret;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 == *s2++) {
        if (*s1++ == '\0') {
            return 0;
        }
    }
    return *reinterpret_cast<const uint8_t *>(s1) - *reinterpret_cast<const uint8_t *>(--s2);
}

int strncmp(const char *s1, const char *s2, size_t n) {
    if (n == 0) {
        return 0;
    }
    do {
        if (*s1 != *s2++) {
            return static_cast<unsigned char>(*s1) - static_cast<unsigned char>(*--s2);
        }
        if (*s1++ == 0) {
            break;
        }
    } while (--n != 0);
    return 0;
}

char *strncpy(char *dst, const char *src, size_t n) {
    for (; n != 0 && (*dst = *src) != '\0'; dst++, src++, n--) {
    }
    memset(dst, 0, n);
    return dst;
}

size_t strnlen(const char *str, size_t max_len) {
    size_t len = 0;
    for (; len < max_len && *str != '\0'; str++) {
        len++;
    }
    return len;
}

char *strchr(const char *str, int c) {
    char *ret = strchrnul(str, c);
    return *ret == c ? ret : nullptr;
}

char *strchrnul(const char *str, int c) {
    for (auto ch = static_cast<char>(c); *str != '\0' && *str != ch; str++) {
    }
    return const_cast<char *>(str);
}

char *strrchr(const char *str, int c) {
    auto ch = static_cast<char>(c);
    for (size_t n = strlen(str) + 1; n != 0; n--) {
        if (str[n] == ch) {
            return const_cast<char *>(str + n);
        }
    }
    return nullptr;
}

size_t strcspn(const char *str, const char *reject) {
    for (const auto *sp = str;;) {
        const auto *rp = reject;
        char ch = *sp++;
        char rc = '\0';
        do {
            if ((rc = *rp++) == ch) {
                return static_cast<size_t>(sp - 1 - str);
            }
        } while (rc != '\0');
    }
}

size_t strspn(const char *str, const char *accept) {
    const char *sp = str;
cont:
    char ch = *sp++;
    char ac = '\0';
    for (const char *ap = accept; (ac = *ap++) != '\0';) {
        if (ac == ch) {
            // NOLINTNEXTLINE
            goto cont;
        }
    }
    return static_cast<size_t>(sp - 1 - str);
}

char *strpbrk(const char *str, const char *accept) {
    str += strcspn(str, accept);
    return *str != '\0' ? const_cast<char *>(str) : nullptr;
}

char *strstr(const char *haystack, const char *needle) {
    if (needle[0] == '\0') {
        return const_cast<char *>(haystack);
    }
    size_t len = strlen(needle);
    for (; *haystack != '\0'; haystack++) {
        if (strncmp(haystack, needle, len) == 0) {
            return const_cast<char *>(haystack);
        }
    }
    return nullptr;
}

char *strtok(char *str, const char *delim) {
    static char *saved_ptr;
    if (str == nullptr && (str = saved_ptr) == nullptr) {
        return nullptr;
    }
    str += strspn(str, delim);
    if (*str == '\0') {
        return (saved_ptr = nullptr);
    }
    saved_ptr = str + strcspn(str, delim);
    if (*saved_ptr != '\0') {
        *saved_ptr++ = '\0';
    } else {
        saved_ptr = nullptr;
    }
    return str;
}

__END_DECLS
