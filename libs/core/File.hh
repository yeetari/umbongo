#pragma once

#include <core/Watchable.hh>
#include <kernel/api/Types.hh>
#include <ustd/Optional.hh>
#include <ustd/Result.hh>
#include <ustd/Span.hh> // IWYU pragma: keep
#include <ustd/StringView.hh>
#include <ustd/Try.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>

namespace core {

enum class SysError : ssize_t;

using OpenMode = kernel::OpenMode;

class File : public Watchable {
    ustd::Optional<uint32_t> m_fd;

public:
    static ustd::Result<File, SysError> open(ustd::StringView path, OpenMode mode = OpenMode::None);

    File() = default;
    explicit File(uint32_t fd) : m_fd(fd) {}
    File(const File &) = delete;
    File(File &&other) : m_fd(ustd::move(other.m_fd)) {}
    ~File() override;

    File &operator=(const File &) = delete;
    File &operator=(File &&other) {
        close();
        m_fd = ustd::move(other.m_fd);
        return *this;
    }

    void close();
    ustd::Result<size_t, SysError> ioctl(kernel::IoctlRequest request, void *arg = nullptr);
    ustd::Result<uintptr_t, SysError> mmap();
    ustd::Result<size_t, SysError> read(ustd::Span<void> data);
    ustd::Result<size_t, SysError> read(ustd::Span<void> data, size_t offset);
    ustd::Result<void, SysError> rebind(uint32_t fd);
    ustd::Result<size_t, SysError> size();
    ustd::Result<size_t, SysError> write(ustd::Span<const void> data);

    template <typename T>
    ustd::Result<T *, SysError> mmap();
    template <typename T>
    ustd::Result<T, SysError> read();
    template <typename T>
    ustd::Result<size_t, SysError> write(const T &data);

    uint32_t fd() const override { return *m_fd; }
};

template <typename T>
ustd::Result<T *, SysError> File::mmap() {
    return reinterpret_cast<T *>(TRY(mmap()));
}

template <typename T>
ustd::Result<T, SysError> File::read() {
    T ret{};
    TRY(read({&ret, sizeof(T)}));
    return ret;
}

template <typename T>
ustd::Result<size_t, SysError> File::write(const T &data) {
    return TRY(write({&data, sizeof(T)}));
}

} // namespace core
