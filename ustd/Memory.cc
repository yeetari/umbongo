#include <ustd/Memory.hh>

#include <ustd/Types.hh>

extern "C" void *memset(void *bufptr, int value, usize size) {
    auto *buf = static_cast<unsigned char *>(bufptr);
    for (usize i = 0; i < size; i++) {
        buf[i] = static_cast<unsigned char>(value);
    }
    return bufptr;
}
