#pragma once

#include <ustd/Types.hh>

extern "C" void *memset(void *, int, usize);

inline void *operator new(usize, void *ptr) {
    return ptr;
}
