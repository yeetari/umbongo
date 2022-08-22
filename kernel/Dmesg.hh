#pragma once

#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/Concepts.hh> // IWYU pragma: keep
#include <ustd/String.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>

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
    ustd::Array<char, 24> buf{};
    uint8 len = 0;
    if (opts[1] == 's') {
        buf[len++] = 'B';
        if (static_cast<usize>(arg) >= 1_TiB) {
            arg /= 1_TiB;
            buf[len++] = 'i';
            buf[len++] = 'T';
        } else if (static_cast<usize>(arg) >= 1_GiB) {
            arg /= 1_GiB;
            buf[len++] = 'i';
            buf[len++] = 'G';
        } else if (static_cast<usize>(arg) >= 1_MiB) {
            arg /= 1_MiB;
            buf[len++] = 'i';
            buf[len++] = 'M';
        } else if (static_cast<usize>(arg) >= 1_KiB) {
            arg /= 1_KiB;
            buf[len++] = 'i';
            buf[len++] = 'K';
        }
        buf[len++] = ' ';
    }
    const usize base = opts[1] == 'h' || opts[1] == 'x' ? 16 : 10;
    do {
        const char digit = static_cast<char>(static_cast<usize>(arg) % base);
        buf[len++] = (digit < 10 ? '0' + digit : 'a' + digit - 10);
        arg /= base;
    } while (arg > 0);

    char pad = opts[2];
    if (pad != '\0') {
        for (uint8 i = len; i < pad - '0'; i++) {
            buf[len++] = opts[3] != '\0' ? opts[3] : '0';
        }
    }
    if (opts[1] == 'h') {
        buf[len++] = 'x';
        buf[len++] = '0';
    }
    for (uint8 i = len; i > 0; i--) {
        dmesg_put_char(buf[i - 1]);
    }
}

template <ustd::Pointer T>
void dmesg_single(const char *, T arg) {
    dmesg_single(":h", reinterpret_cast<uintptr>(arg));
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
    for (uint32 i = 0; *fmt != '}';) {
        ASSERT(i < opts.size());
        opts[i++] = *fmt++;
    }
    dmesg_single(opts.data(), arg);
    fmt++;
}

template <typename... Args>
void dmesg(const char *fmt, const Args &...args) {
    dmesg_lock();
    (dmesg_part(fmt, args), ...);
    while (*fmt != '\0') {
        dmesg_put_char(*fmt++);
    }
    dmesg_put_char('\n');
    dmesg_unlock();
}

} // namespace kernel
