#include <ustd/Memory.hh>

#include <ustd/Types.hh>

extern "C" int memcmp(const void *aptr, const void *bptr, usize size) {
    const auto *a = static_cast<const unsigned char *>(aptr);
    const auto *b = static_cast<const unsigned char *>(bptr);
    for (usize i = 0; i < size; i++) {
        if (a[i] < b[i]) {
            return -1;
        }
        if (b[i] < a[i]) {
            return 1;
        }
    }
    return 0;
}

extern "C" void *memcpy(void *dstptr, const void *srcptr, usize size) {
    auto *dst = static_cast<unsigned char *>(dstptr);
    const auto *src = static_cast<const unsigned char *>(srcptr);
    for (usize i = 0; i < size; i++) {
        dst[i] = src[i];
    }
    return dst;
}

extern "C" void *memset(void *bufptr, int value, usize size) {
    auto *buf = static_cast<unsigned char *>(bufptr);
    for (usize i = 0; i < size; i++) {
        buf[i] = static_cast<unsigned char>(value);
    }
    return bufptr;
}

extern "C" int strcmp(const char *s1, const char *s2) {
    while (*s1 == *s2++) {
        if (*s1++ == '\0') {
            return 0;
        }
    }
    return *reinterpret_cast<const unsigned char *>(s1) - *reinterpret_cast<const unsigned char *>(--s2);
}

extern "C" usize strlen(const char *str) {
    usize len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

extern "C" usize wstrlen(const wchar_t *str) {
    usize len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}
