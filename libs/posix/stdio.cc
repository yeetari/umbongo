#include <stdio.h>

#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include <kernel/SysError.hh>
#include <kernel/Syscall.hh>
#include <kernel/SyscallTypes.hh>
#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/Log.hh>
#include <ustd/Memory.hh>
#include <ustd/Numeric.hh>
#include <ustd/Span.hh>
#include <ustd/StringView.hh>

struct FILE {
private:
    const uint32_t m_fd;
    int m_error{0};
    bool m_eof{false};

public:
    explicit FILE(uint32_t fd) : m_fd(fd) {}
    FILE(const FILE &) = delete;
    FILE(FILE &&) = delete;
    ~FILE() { Syscall::invoke(Syscall::close, m_fd); }

    FILE &operator=(const FILE &) = delete;
    FILE &operator=(FILE &&) = delete;

    bool gets(uint8_t *data, int size);
    size_t read(void *data, size_t size);
    int seek(long offset, int whence);
    ssize_t tell();
    size_t write(const void *data, size_t size);

    uint32_t fd() const { return m_fd; }
    int error() const { return m_error; }
    bool eof() const { return m_eof; }
};

namespace {

// NOLINTNEXTLINE
alignas(FILE) uint8_t s_default_streams[3][sizeof(FILE)];

int printf_impl(char *str, const char *fmt, va_list ap, FILE *stream) {
    auto put_char = [&](char ch) {
        if (str != nullptr) {
            *str++ = ch;
        } else {
            fputc(ch, stream);
        }
    };
    size_t len = 0;
    for (const char *p = fmt; *p != '\0'; p++) {
        if (*p != '%' || *(p + 1) == '\0') {
            put_char(*p);
            len++;
            continue;
        }

        bool has_dot = false;
        bool left_pad = false;
        size_t long_qualifiers = 0;
        size_t field_width = 0;
        auto put_string = [&](StringView string) {
            if (!has_dot && (field_width == 0 || field_width < string.length())) {
                field_width = string.length();
            }
            size_t pad_amount = field_width > string.length() ? field_width - string.length() : 0;
            if (!left_pad) {
                for (size_t i = 0; i < pad_amount; i++) {
                    put_char(' ');
                }
            }
            for (size_t i = 0; i < ustd::min(string.length(), field_width); i++) {
                put_char(string[i]);
            }
            if (left_pad) {
                for (size_t i = 0; i < pad_amount; i++) {
                    put_char(' ');
                }
            }
            len += field_width;
        };

    has_more:
        p++;
        if (*p == '.') {
            has_dot = true;
            if (*(p + 1) != '\0') {
                // NOLINTNEXTLINE
                goto has_more;
            }
        }
        if (*p == '-') {
            left_pad = true;
            if (*(p + 1) != '\0') {
                // NOLINTNEXTLINE
                goto has_more;
            }
        }
        if (*p == '*') {
            field_width = va_arg(ap, size_t);
            if (*(p + 1) != '\0') {
                // NOLINTNEXTLINE
                goto has_more;
            }
        }
        if (*p >= '0' && *p <= '9') {
            field_width *= 10;
            field_width += static_cast<size_t>(*p - '0');
            if (*(p + 1) != '\0') {
                // NOLINTNEXTLINE
                goto has_more;
            }
        }
        if (*p == 'l') {
            long_qualifiers++;
            if (*(p + 1) != '\0') {
                // NOLINTNEXTLINE
                goto has_more;
            }
        }
        switch (*p) {
        case '%':
            put_char('%');
            len++;
            break;
        case 'c': {
            auto ch = static_cast<char>(va_arg(ap, int));
            put_string({&ch, 1});
            break;
        }
        case 'd':
        case 'u':
        case 'x': {
            Array<char, 20> buf{};
            bool negative = false;
            size_t num = 0;
            size_t num_len = 0;
            switch (*p) {
            case 'd': {
                ssize_t signed_num = long_qualifiers >= 2 ? va_arg(ap, int64_t) : va_arg(ap, int32_t);
                if (signed_num < 0) {
                    negative = true;
                    signed_num = -signed_num;
                }
                num = static_cast<size_t>(signed_num);
                break;
            }
            case 'u':
            case 'x':
                num = long_qualifiers >= 2 ? va_arg(ap, uint64_t) : va_arg(ap, uint32_t);
                break;
            default:
                ENSURE_NOT_REACHED();
            }
            const size_t base = *p == 'x' ? 16 : 10;
            do {
                const auto digit = static_cast<char>(num % base);
                buf[num_len++] = static_cast<char>(digit < 10 ? '0' + digit : 'a' + digit - 10);
                num /= base;
            } while (num > 0);
            if (base == 16) {
                buf[num_len++] = 'x';
                buf[num_len++] = '0';
            }
            if (negative) {
                buf[num_len++] = '-';
            }
            for (size_t i = 0; i < num_len / 2; i++) {
                char tmp = buf[i];
                buf[i] = buf[num_len - i - 1];
                buf[num_len - i - 1] = tmp;
            }
            put_string({buf.data(), num_len});
            break;
        }
        case 's': {
            const auto *string = va_arg(ap, const char *);
            put_string(string);
            break;
        }
        default:
            dbgln("Unrecognised format specifier {:c}", *p);
            ENSURE_NOT_REACHED();
        }
    }

    if (str != nullptr) {
        *str = '\0';
    }
    return static_cast<int>(len);
}

int scanf_impl(const char *str, [[maybe_unused]] const char *fmt, va_list ap) {
    // TODO: Implement properly.
    ASSERT(strcmp(fmt, "%lu") == 0);
    *va_arg(ap, size_t *) = static_cast<size_t>(atoi(str));
    return 1;
}

OpenMode parse_mode(const char *mode) {
    auto ret = OpenMode::None;
    for (const auto *ch = mode; *ch != '\0'; ch++) {
        switch (*ch) {
        case 'r':
        case '+':
            break;
        case 'w':
            ret |= OpenMode::Create | OpenMode::Truncate;
            break;
        case 'b':
        case 't':
            break;
        default:
            dbgln("Unknown mode {:c}", *ch);
            ENSURE_NOT_REACHED();
        }
    }
    return ret;
}

int to_errno(ssize_t error) {
    switch (static_cast<SysError>(error)) {
    case SysError::BadFd:
        return EBADF;
    case SysError::NonExistent:
        return ENOENT;
    case SysError::BrokenHandle:
        return EIO;
    case SysError::Invalid:
        return EINVAL;
    default:
        ENSURE_NOT_REACHED();
    }
}

SeekMode seek_mode(int whence) {
    switch (whence) {
    case SEEK_SET:
        return SeekMode::Set;
    case SEEK_CUR:
        return SeekMode::Add;
    default:
        ENSURE_NOT_REACHED();
    }
}

} // namespace

bool FILE::gets(uint8_t *data, int size) {
    if (size == 0) {
        return false;
    }
    size_t total_read = 0;
    while (size > 1) {
        uint8_t byte = 0;
        auto bytes_read = Syscall::invoke(Syscall::read, m_fd, &byte, 1);
        if (bytes_read <= 0) {
            *data = 0;
            if (bytes_read == 0) {
                m_eof = true;
            } else {
                m_error = to_errno(bytes_read);
            }
            return total_read > 0;
        }
        *data = byte;
        total_read++;
        data++;
        size--;
    }
    *data = 0;
    return total_read > 0;
}

size_t FILE::read(void *data, size_t size) {
    size_t total_read = 0;
    while (size > 0) {
        auto bytes_read = Syscall::invoke(Syscall::read, m_fd, data, size);
        if (bytes_read <= 0) {
            if (bytes_read == 0) {
                m_eof = true;
            } else {
                m_error = to_errno(bytes_read);
            }
            return total_read;
        }
        total_read += static_cast<size_t>(bytes_read);
        size -= static_cast<size_t>(bytes_read);
    }
    return total_read;
}

int FILE::seek(long offset, int whence) {
    ENSURE(whence != SEEK_END, "TODO: Support SEEK_END");
    ssize_t rc = Syscall::invoke(Syscall::seek, m_fd, offset, seek_mode(whence));
    if (rc < 0) {
        return -1;
    }
    m_eof = false;
    return 0;
}

ssize_t FILE::tell() {
    return Syscall::invoke(Syscall::seek, m_fd, 0, SeekMode::Add);
}

size_t FILE::write(const void *data, size_t size) {
    size_t total_written = 0;
    while (size > 0) {
        auto bytes_written = Syscall::invoke(Syscall::write, m_fd, data, size);
        if (bytes_written < 0) {
            m_error = to_errno(bytes_written);
            return total_written;
        }
        total_written += static_cast<size_t>(bytes_written);
        size -= static_cast<size_t>(bytes_written);
    }
    return total_written;
}

__BEGIN_DECLS

FILE *fopen(const char *path, const char *mode) {
    auto fd = Syscall::invoke(Syscall::open, path, parse_mode(mode));
    if (fd < 0) {
        return nullptr;
    }
    return new FILE(static_cast<uint32_t>(fd));
}

int feof(FILE *stream) {
    ASSERT(stream != nullptr);
    return stream->eof();
}

int ferror(FILE *stream) {
    ASSERT(stream != nullptr);
    return stream->error();
}

int fflush(FILE *stream) {
    if (stream == nullptr) {
        ENSURE_NOT_REACHED("TODO: Flush all open streams");
    }
    return 0;
}

int fclose(FILE *stream) {
    delete stream;
    return 0;
}

int fseek(FILE *stream, long offset, int whence) {
    ASSERT(stream != nullptr);
    return stream->seek(offset, whence);
}

long ftell(FILE *stream) {
    ASSERT(stream != nullptr);
    return stream->tell();
}

void rewind(FILE *stream) {
    fseek(stream, 0l, SEEK_SET);
}

char *fgets(char *data, int size, FILE *stream) {
    ASSERT(stream != nullptr);
    bool success = stream->gets(reinterpret_cast<uint8_t *>(data), size);
    return success ? data : nullptr;
}

size_t fread(void *data, size_t size, size_t count, FILE *stream) {
    ASSERT(stream != nullptr);
    size_t bytes_read = stream->read(data, size * count);
    return bytes_read / size;
}

size_t fwrite(const void *data, size_t size, size_t count, FILE *stream) {
    ASSERT(stream != nullptr);
    size_t bytes_written = stream->write(data, size * count);
    return bytes_written / size;
}

int fgetc(FILE *stream) {
    char ch = '\0';
    return fread(&ch, 1, 1, stream) == 1 ? ch : EOF;
}

int fputc(int c, FILE *stream) {
    ASSERT(stream != nullptr);
    auto ch = static_cast<uint8_t>(c);
    return stream->write(&ch, 1) == 1 ? static_cast<int>(ch) : EOF;
}

int fputs(const char *str, FILE *stream) {
    ASSERT(stream != nullptr);
    size_t len = strlen(str);
    return stream->write(str, len) < len ? EOF : 1;
}

int getc(FILE *stream) {
    return fgetc(stream);
}

int putc(int c, FILE *stream) {
    return fputc(c, stream);
}

int puts(const char *str) {
    if (fputs(str, stdout) == EOF) {
        return EOF;
    }
    return fputc('\n', stdout);
}

int ungetc(int, FILE *) {
    ENSURE_NOT_REACHED();
}

void clearerr(FILE *) {
    ENSURE_NOT_REACHED();
}

void perror(const char *) {
    ENSURE_NOT_REACHED();
}

int fileno(FILE *stream) {
    ASSERT(stream != nullptr);
    return static_cast<int>(stream->fd());
}

int getchar(void) {
    ENSURE_NOT_REACHED();
}

int putchar(int c) {
    return putc(c, stdout);
}

FILE *fdopen(int, const char *) {
    ENSURE_NOT_REACHED();
}

int printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int ret = vprintf(fmt, ap);
    va_end(ap);
    return ret;
}

int fprintf(FILE *stream, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int ret = vfprintf(stream, fmt, ap);
    va_end(ap);
    return ret;
}

int sprintf(char *str, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int ret = vsprintf(str, fmt, ap);
    va_end(ap);
    return ret;
}

int vprintf(const char *fmt, va_list ap) {
    return vfprintf(stdout, fmt, ap);
}

int vfprintf(FILE *stream, const char *fmt, va_list ap) {
    return printf_impl(nullptr, fmt, ap, stream);
}

int vsprintf(char *str, const char *fmt, va_list ap) {
    return printf_impl(str, fmt, ap, nullptr);
}

int scanf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int ret = vscanf(fmt, ap);
    va_end(ap);
    return ret;
}

int fscanf(FILE *stream, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int ret = vfscanf(stream, fmt, ap);
    va_end(ap);
    return ret;
}

int sscanf(const char *str, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int ret = vsscanf(str, fmt, ap);
    va_end(ap);
    return ret;
}

int vscanf(const char *fmt, va_list ap) {
    return vfscanf(stdin, fmt, ap);
}

int vfscanf(FILE *, const char *, va_list) {
    ENSURE_NOT_REACHED("TODO");
}

int vsscanf(const char *str, const char *fmt, va_list ap) {
    return scanf_impl(str, fmt, ap);
}

int remove(const char *) {
    ENSURE_NOT_REACHED();
}

FILE *tmpfile() {
    ENSURE_NOT_REACHED();
}

FILE *stdin = reinterpret_cast<FILE *>(&s_default_streams[0]);
FILE *stdout = reinterpret_cast<FILE *>(&s_default_streams[1]);
FILE *stderr = reinterpret_cast<FILE *>(&s_default_streams[2]);

void __stdio_init() {
    new (stdin) FILE(0);
    new (stdout) FILE(1);
    new (stderr) FILE(1);
}

__END_DECLS
