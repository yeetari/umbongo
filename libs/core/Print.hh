#pragma once

#include <core/Syscall.hh>
#include <ustd/Format.hh>
#include <ustd/Utility.hh>

namespace core {

void put_char(char);

template <typename... Args>
void print(const char *fmt, Args &&...args) {
    auto message = ustd::format(fmt, ustd::forward<Args>(args)...);
    static_cast<void>(core::syscall(Syscall::write, 1, message.data(), message.length()));
}

template <typename... Args>
void println(const char *fmt, Args &&...args) {
    print(fmt, ustd::forward<Args>(args)...);
    put_char('\n');
}

inline void put_char(char ch) {
    static_cast<void>(core::syscall(Syscall::write, 1, &ch, 1));
}

} // namespace core