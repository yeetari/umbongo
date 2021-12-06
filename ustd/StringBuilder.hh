#pragma once

#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/Concepts.hh> // IWYU pragma: keep
#include <ustd/String.hh>
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

namespace ustd {

class StringBuilder {
    LargeVector<char> m_buffer;

    template <Integral T>
    void append_single(const char *opts, T arg);
    void append_single(const char *opts, const String &arg);
    template <typename T>
    void append_part(const char *&fmt, const T &arg);

public:
    template <typename... Args>
    void append(const char *fmt, const Args &...args);

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
    const usize base = opts[1] == 'h' || opts[1] == 'x' ? 16 : 10;
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
    if (opts[1] == 'h') {
        buf[len++] = 'x';
        buf[len++] = '0';
    }
    for (uint8 i = len; i > 0; i--) {
        m_buffer.push(buf[i - 1]);
    }
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
    uint32 i = 0;
    while (*fmt != '}') {
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