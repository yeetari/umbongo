#pragma once

#include <system/Syscall.hh>
#include <ustd/Format.hh>
#include <ustd/StringView.hh>
#include <ustd/Try.hh>
#include <ustd/Utility.hh>

namespace core {

inline void debug_line(ustd::StringView message) {
    ASSUME(system::syscall(SYS_debug_line, message.data(), message.length()));
}

template <typename... Args>
void debug_line(const char *fmt, Args &&...args) {
    auto message = ustd::format(fmt, ustd::forward<Args>(args)...);
    ASSUME(system::syscall(SYS_debug_line, message.data(), message.length()));
}

} // namespace core
