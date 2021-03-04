#pragma once

#include <ustd/Array.hh>
#include <ustd/Traits.hh>
#include <ustd/Types.hh>

void put_char(char ch);

namespace ustd {
namespace detail {

template <typename T>
void log_single(const char *, T);

template <typename T>
void log_single(const char *opts, T arg) requires IsInteger<T> {
    if (opts[1] == 'c') {
        put_char(static_cast<char>(arg));
        return;
    }
    const usize base = opts[1] == 'h' ? 16 : 10;
    Array<char, 20> buf{};
    uint32 len = 0;
    do {
        const char digit = static_cast<char>(arg % base);
        buf[len++] = (digit < 10 ? '0' + digit : 'a' + digit - 10);
        arg /= base;
    } while (arg > 0);

    if (base == 16) {
        buf[len++] = 'x';
        buf[len++] = '0';
    }

    for (uint32 i = len; i > 0; i--) {
        put_char(buf[i - 1]);
    }
}

template <typename T>
void log_single(const char *, T arg) requires IsPointer<T> {
    log_single(":h", reinterpret_cast<uintptr>(arg));
}

template <>
inline void log_single(const char *, const char *msg) {
    while (*msg != '\0') {
        put_char(*msg++);
    }
}

template <>
inline void log_single(const char *, bool arg) {
    log_single("", arg ? "true" : "false");
}

template <typename T>
// NOLINTNEXTLINE
void log_part(const char *&fmt, const T &arg) {
    while (*fmt != '\0' && *fmt != '{') {
        put_char(*fmt++);
    }
    if (*fmt == '{') {
        Array<char, 4> opts{};
        fmt++;
        uint32 i = 0;
        while (*fmt != '}') {
            // TODO: Assert not more than 4 chars inside braces.
            opts[i++] = *fmt++;
        }
        log_single(opts.data(), arg);
        fmt++;
    }
}

// NOLINTNEXTLINE
inline void log_part(const char *&fmt) {
    while (*fmt != '\0') {
        put_char(*fmt++);
    }
}

} // namespace detail

template <typename... Args>
void log(const char *fmt, const Args &...args) {
    (detail::log_part(fmt, args), ...);
    detail::log_part(fmt);
}

template <typename... Args>
void logln(const char *fmt, const Args &...args) {
    log(fmt, args...);
    put_char('\n');
}

} // namespace ustd

using ustd::log;
using ustd::logln;
