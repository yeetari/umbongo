#pragma once

#include <ustd/array.hh>
#include <ustd/assert.hh>
#include <ustd/numeric.hh>
#include <ustd/string.hh>
#include <ustd/string_view.hh>
#include <ustd/types.hh>
#include <ustd/utility.hh>
#include <ustd/vector.hh>

namespace ustd {

class StringBuilder {
    LargeVector<char> m_buffer;

    template <Integral T>
    void append_single(const char *opts, T arg);
    template <Pointer T>
    void append_single(const char *opts, T arg);
    void append_single(const char *opts, const char *arg);
    void append_single(const char *opts, StringView arg);
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
    const size_t base = opts[1] == 'h' || opts[1] == 'x' ? 16 : 10;
    Array<char, 20> buf{};
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
    if (opts[1] == 'h') {
        buf[len++] = 'x';
        buf[len++] = '0';
    }
    for (uint8_t i = len; i > 0; i--) {
        m_buffer.push(buf[i - 1]);
    }
}

template <Pointer T>
void StringBuilder::append_single(const char *, T arg) {
    append_single(":h", reinterpret_cast<uintptr_t>(arg));
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
    for (uint32_t i = 0; *fmt != '}';) {
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

template <typename... Args>
String format(const char *fmt, Args &&...args) {
    StringBuilder builder;
    builder.append(fmt, forward<Args>(args)...);
    return builder.build();
}

} // namespace ustd
