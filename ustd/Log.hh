#pragma once

#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/StringView.hh>
#include <ustd/Traits.hh>
#include <ustd/Types.hh>

void dbg_put_char(char ch);
void put_char(char ch);
[[gnu::weak]] inline void log_lock() {}
[[gnu::weak]] inline void log_unlock() {}

namespace ustd {
namespace detail {

using LogFn = void (*)(char);

template <typename T>
void log_single(LogFn fn, const char *, T);

template <typename T>
void log_single(LogFn fn, const char *opts, T arg) requires IsIntegral<T> {
    if (opts[1] == 'c') {
        fn(static_cast<char>(arg));
        return;
    }
    const usize base = opts[1] == 'h' ? 16 : 10;
    Array<char, 20> buf{};
    uint8 len = 0;
    do {
        const char digit = static_cast<char>(static_cast<usize>(arg) % base);
        buf[len++] = (digit < 10 ? '0' + digit : 'a' + digit - 10);
        arg /= base;
    } while (arg > 0);

    char pad = opts[2];
    if (pad != '\0') {
        for (uint8 i = len; i < pad - '0'; i++) {
            buf[len++] = '0';
        }
    }

    if (base == 16) {
        buf[len++] = 'x';
        buf[len++] = '0';
    }

    for (uint8 i = len; i > 0; i--) {
        fn(buf[i - 1]);
    }
}

template <typename T>
void log_single(LogFn fn, const char *, T arg) requires IsPointer<T> {
    log_single(fn, ":h", reinterpret_cast<uintptr>(arg));
}

template <>
inline void log_single(LogFn fn, const char *, const char *msg) {
    while (*msg != '\0') {
        fn(*msg++);
    }
}

template <>
inline void log_single(LogFn fn, const char *, StringView msg) {
    for (char ch : msg) {
        fn(ch);
    }
}

template <>
inline void log_single(LogFn fn, const char *, bool arg) {
    log_single(fn, "", arg ? "true" : "false");
}

template <typename T>
// NOLINTNEXTLINE
void log_part(LogFn fn, const char *&fmt, const T &arg) {
    while (*fmt != '\0' && *fmt != '{') {
        fn(*fmt++);
    }
    if (*fmt == '{') {
        Array<char, 4> opts{};
        fmt++;
        uint32 i = 0;
        while (*fmt != '}') {
            ASSERT(i < opts.size());
            opts[i++] = *fmt++;
        }
        log_single(fn, opts.data(), arg);
        fmt++;
    }
}

// NOLINTNEXTLINE
inline void log_part(LogFn fn, const char *&fmt) {
    while (*fmt != '\0') {
        fn(*fmt++);
    }
}

} // namespace detail

template <typename... Args>
void dbg(const char *fmt, const Args &...args) {
    (detail::log_part(&dbg_put_char, fmt, args), ...);
    detail::log_part(&dbg_put_char, fmt);
}

template <typename... Args>
void dbgln(const char *fmt, const Args &...args) {
    log_lock();
    dbg(fmt, args...);
    dbg_put_char('\n');
    log_unlock();
}

template <typename... Args>
void printf(const char *fmt, const Args &...args) {
    log_lock();
    (detail::log_part(&put_char, fmt, args), ...);
    detail::log_part(&put_char, fmt);
    log_unlock();
}

} // namespace ustd
