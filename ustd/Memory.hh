#pragma once

#include <ustd/Types.hh>

extern "C" int memcmp(const void *, const void *, usize);
extern "C" void *memcpy(void *, const void *, usize);
extern "C" void *memset(void *, int, usize);
extern "C" int strcmp(const char *, const char *);
extern "C" usize strlen(const char *);
extern "C" usize wstrlen(const wchar_t *);

inline void *operator new(usize, void *ptr) {
    return ptr;
}

inline void *operator new[](usize, void *ptr) {
    return ptr;
}
