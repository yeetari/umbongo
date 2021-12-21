#pragma once

#include <ustd/Types.hh>

namespace std {

enum class align_val_t : usize {};

} // namespace std

namespace ustd {

using align_val_t = std::align_val_t;

} // namespace ustd

inline void *operator new(usize, void *ptr) {
    return ptr;
}

inline void *operator new[](usize, void *ptr) {
    return ptr;
}
