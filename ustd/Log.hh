#pragma once

#include <ustd/Array.hh>
#include <ustd/Traits.hh>
#include <ustd/Types.hh>

void put_char(char ch);

namespace detail {

template <typename T>
void log_single(const char *, const T &);

template <typename T>
void log_single(const char *opts, const T &arg) requires IsInteger<T> {
    const usize base = opts[1] == 'h' ? 16 : 10;
    Array<char, 20> buf{};
    uint32 len = 0;
    T val = arg;
    do {
        const char digit = static_cast<char>(val % base);
        buf[len++] = (digit < 10 ? '0' + digit : 'A' + digit - 10);
        val /= base;
    } while (val > 0);

    if (base == 16) {
        buf[len++] = 'x';
        buf[len++] = '0';
    }

    for (uint32 i = len; i > 0; i--) {
        put_char(buf[i - 1]);
    }
}

template <typename T>
void log_single(const char *, const T &arg) requires IsPointer<T> {
    log_single(":h", reinterpret_cast<uintptr>(arg));
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
void log_part(const char *&fmt) {
    while (*fmt != '\0') {
        put_char(*fmt++);
    }
}

} // namespace detail

template <typename... Args>
void log(const char *fmt, const Args &... args) {
    if constexpr (sizeof...(args) == 0) {
        detail::log_part(fmt);
    }
    (detail::log_part(fmt, args), ...);
    put_char('\n');
}
