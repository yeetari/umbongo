#pragma once

#include <ustd/Types.hh>

extern "C" int memcmp(const void *, const void *, usize);
extern "C" void *memset(void *, int, usize);

inline void *operator new(usize, void *ptr) {
    return ptr;
}

inline void *operator new[](usize, void *ptr) {
    return ptr;
}
