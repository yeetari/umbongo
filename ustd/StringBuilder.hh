#pragma once

#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/Concepts.hh> // IWYU pragma: keep
#include <ustd/String.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

namespace ustd {

class StringBuilder {
    LargeVector<char> m_buffer;

    template <Integral T>
    void append_single(const char *opts, T arg);
    template <Pointer T>
    void append_single(const char *opts, T arg);
    void append_single(const char *opts, const char *arg);
    void append_single(const char *opts, StringView arg);
    void append_single(const char *opts, const String &arg);
    void append_single(const char *opts, bool arg);
    template <typename T>
    void append_part(const char *&fmt, const T &arg);

public:
    template <typename... Args>
    void append(const char *fmt, const Args &...args);
    void append(char ch);

    String build();
    String build_copy() const;
    auto length() const { return m_buffer.size(); }
};

template <Integral T>
void StringBuilder::append_single(const char *opts, T arg) {
    if (opts[1] == 'c') {
        m_buffer.push(static_cast<char>(arg));
        return;
    }
    Array<char, 24> buf{};
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
        m_buffer.push(buf[i - 1]);
    }
}

template <Pointer T>
void StringBuilder::append_single(const char *, T arg) {
    append_single(":h", reinterpret_cast<uintptr>(arg));
}

template <typename T>
void StringBuilder::append_part(const char *&fmt, const T &arg) {
    while (*fmt != '\0' && *fmt != '{') {
        m_buffer.push(*fmt++);
    }
    if (*fmt++ != '{') {
        return;
    }
    Array<char, 4> opts{};
    for (uint32 i = 0; *fmt != '}';) {
        ASSERT(i < opts.size());
        opts[i++] = *fmt++;
    }
    append_single(opts.data(), arg);
    fmt++;
}

template <typename... Args>
void StringBuilder::append(const char *fmt, const Args &...args) {
    (append_part(fmt, args), ...);
    while (*fmt != '\0') {
        m_buffer.push(*fmt++);
    }
}

} // namespace ustd
