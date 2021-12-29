#pragma once

#include <core/Error.hh>
#include <core/Watchable.hh>
#include <kernel/SyscallTypes.hh>
#include <ustd/Optional.hh>
#include <ustd/Result.hh>
#include <ustd/Span.hh> // IWYU pragma: keep
#include <ustd/StringView.hh>
#include <ustd/Try.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>

namespace core {

using OpenMode = kernel::OpenMode;

class File : public Watchable {
    ustd::Optional<uint32> m_fd;

public:
    static ustd::Result<File, SysError> open(ustd::StringView path, OpenMode mode = OpenMode::None);

    File() = default;
    explicit File(uint32 fd) : m_fd(fd) {}
    File(const File &) = delete;
    File(File &&other) noexcept : m_fd(ustd::move(other.m_fd)) {}
    ~File() override;

    File &operator=(const File &) = delete;
    File &operator=(File &&other) noexcept {
        close();
        m_fd = ustd::move(other.m_fd);
        return *this;
    }

    void close();
    ustd::Result<usize, SysError> ioctl(kernel::IoctlRequest request, void *arg = nullptr);
    ustd::Result<uintptr, SysError> mmap();
    ustd::Result<usize, SysError> read(ustd::Span<void> data);
    ustd::Result<usize, SysError> read(ustd::Span<void> data, usize offset);
    ustd::Result<void, SysError> rebind(uint32 fd);
    ustd::Result<usize, SysError> size();
    ustd::Result<usize, SysError> write(ustd::Span<const void> data);

    template <typename T>
    ustd::Result<T *, SysError> mmap();
    template <typename T>
    ustd::Result<T, SysError> read();
    template <typename T>
    ustd::Result<usize, SysError> write(const T &data);

    uint32 fd() const override { return *m_fd; }
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
ustd::Result<usize, SysError> File::write(const T &data) {
    return TRY(write({&data, sizeof(T)}));
}

} // namespace core
