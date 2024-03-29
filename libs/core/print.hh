#pragma once

#include <system/syscall.hh>
#include <system/system.h>
#include <ustd/string_builder.hh>
#include <ustd/utility.hh>

namespace core {

void put_char(char);

template <typename... Args>
void print(const char *fmt, Args &&...args) {
    auto message = ustd::format(fmt, ustd::forward<Args>(args)...);
    static_cast<void>(system::syscall(UB_SYS_write, 1, message.data(), message.length()));
}

template <typename... Args>
void println(const char *fmt, Args &&...args) {
    print(fmt, ustd::forward<Args>(args)...);
    put_char('\n');
}

inline void put_char(char ch) {
    static_cast<void>(system::syscall(UB_SYS_write, 1, &ch, 1));
}

} // namespace core
