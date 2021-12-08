#pragma once

#include <ustd/String.hh>
#include <ustd/StringBuilder.hh>

namespace ustd {

template <typename... Args>
String format(const char *fmt, Args &&...args) {
    StringBuilder builder;
    builder.append(fmt, forward<Args>(args)...);
    return builder.build();
}

} // namespace ustd
