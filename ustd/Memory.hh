#pragma once

#include <ustd/Types.hh>

namespace std {

enum class align_val_t : usize {};

} // namespace std

namespace ustd {

using align_val_t = std::align_val_t;

} // namespace ustd

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
