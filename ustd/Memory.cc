#include <ustd/Memory.hh>

#include <ustd/Types.hh>

extern "C" int memcmp(const void *aptr, const void *bptr, size_t size) {
    const auto *a = static_cast<const uint8_t *>(aptr);
    const auto *b = static_cast<const uint8_t *>(bptr);
    for (size_t i = 0; i < size; i++) {
        if (a[i] < b[i]) {
            return -1;
        }
        if (b[i] < a[i]) {
            return 1;
        }
    }
    return 0;
}

extern "C" void *memcpy(void *dstptr, const void *srcptr, size_t size) {
    auto *dst = static_cast<uint8_t *>(dstptr);
    const auto *src = static_cast<const uint8_t *>(srcptr);
    for (size_t i = 0; i < size; i++) {
        dst[i] = src[i];
    }
    return dst;
}

extern "C" void *memset(void *bufptr, int value, size_t size) {
    auto *buf = static_cast<uint8_t *>(bufptr);
    for (size_t i = 0; i < size; i++) {
        buf[i] = static_cast<uint8_t>(value);
    }
    return bufptr;
}

extern "C" size_t strlen(const char *str) {
    size_t len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}
