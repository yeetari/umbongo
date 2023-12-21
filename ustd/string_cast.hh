#pragma once

#include <ustd/numeric.hh>
#include <ustd/optional.hh>
#include <ustd/string_view.hh>

namespace ustd {

template <UnsignedIntegral T>
Optional<T> cast(StringView string) {
    T ret = 0;
    for (auto ch : string) {
        if (ch < '0' || ch > '9') {
            return {};
        }
        if (__builtin_mul_overflow(ret, T(10), &ret)) {
            return {};
        }
        if (__builtin_add_overflow(ret, ch - '0', &ret)) {
            return {};
        }
    }
    return ret;
}

} // namespace ustd
