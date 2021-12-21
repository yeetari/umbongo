#include <ustd/Memory.hh>

#include <ustd/Types.hh>

extern "C" int memcmp(const void *aptr, const void *bptr, usize size) {
    const auto *a = static_cast<const uint8 *>(aptr);
    const auto *b = static_cast<const uint8 *>(bptr);
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
    auto *dst = static_cast<uint8 *>(dstptr);
    const auto *src = static_cast<const uint8 *>(srcptr);
    for (usize i = 0; i < size; i++) {
        dst[i] = src[i];
    }
    return dst;
}

extern "C" void *memset(void *bufptr, int value, usize size) {
    auto *buf = static_cast<uint8 *>(bufptr);
    for (usize i = 0; i < size; i++) {
        buf[i] = static_cast<uint8>(value);
    }
    return bufptr;
}

extern "C" usize strlen(const char *str) {
    usize len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}
