#pragma once

#include <ustd/array.hh>
#include <ustd/assert.hh>
#include <ustd/numeric.hh>
#include <ustd/string.hh>
#include <ustd/string_view.hh>
#include <ustd/types.hh>
#include <ustd/utility.hh>

namespace kernel {

void dmesg_lock();
void dmesg_unlock();
void dmesg_put_char(char ch);

template <typename T>
void dmesg_single(const char *, T);
void dmesg_single(const char *, bool arg);
void dmesg_single(const char *, const char *arg);
void dmesg_single(const char *, ustd::StringView arg);
void dmesg_single(const char *, const ustd::String &arg);

template <ustd::Integral T>
void dmesg_single(const char *opts, T arg) {
    if (opts[1] == 'c') {
        dmesg_put_char(static_cast<char>(arg));
        return;
    }
    const size_t base = opts[1] == 'h' || opts[1] == 'p' || opts[1] == 'x' ? 16 : 10;
    ustd::Array<char, 20> buf{};
    uint8_t len = 0;
    do {
        const char digit = static_cast<char>(static_cast<size_t>(arg) % base);
        buf[len++] = (digit < 10 ? '0' + digit : 'a' + digit - 10);
        arg /= base;
    } while (arg > 0);

    char pad = opts[2];
    if (pad != '\0') {
        for (uint8_t i = len; i < pad - '0'; i++) {
            buf[len++] = opts[3] != '\0' ? opts[3] : '0';
        }
    }
    if (opts[1] == 'p') {
        for (uint8_t i = len; i < 12; i++) {
            buf[len++] = '0';
        }
    }
    if (opts[1] == 'h' || opts[1] == 'p') {
        buf[len++] = 'x';
        buf[len++] = '0';
    }
    for (uint8_t i = len; i > 0; i--) {
        dmesg_put_char(buf[i - 1]);
    }
}

template <ustd::Pointer T>
void dmesg_single(const char *, T arg) {
    dmesg_single(":h", reinterpret_cast<uintptr_t>(arg));
}

template <typename T>
void dmesg_part(const char *&fmt, const T &arg) {
    while (*fmt != '\0' && *fmt != '{') {
        dmesg_put_char(*fmt++);
    }
    if (*fmt++ != '{') {
        return;
    }
    ustd::Array<char, 4> opts{};
    for (uint32_t i = 0; *fmt != '}';) {
        ASSERT(i < opts.size());
        opts[i++] = *fmt++;
    }
    dmesg_single(opts.data(), arg);
    fmt++;
}

template <typename... Args>
void dmesg_no_lock(const char *fmt, const Args &...args) {
    (dmesg_part(fmt, args), ...);
    while (*fmt != '\0') {
        dmesg_put_char(*fmt++);
    }
    dmesg_put_char('\n');
}

template <typename... Args>
void dmesg(const char *fmt, const Args &...args) {
    dmesg_lock();
    dmesg_no_lock(fmt, args...);
    dmesg_unlock();
}

} // namespace kernel
