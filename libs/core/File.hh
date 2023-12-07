#pragma once

#include <core/Watchable.hh>
#include <system/Error.h>
#include <system/System.h>
#include <ustd/Optional.hh>
#include <ustd/Result.hh>
#include <ustd/Span.hh> // IWYU pragma: keep
#include <ustd/StringView.hh>
#include <ustd/Try.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>

namespace core {

class File : public Watchable {
    ustd::Optional<uint32_t> m_fd;

public:
    static ustd::Result<File, ub_error_t> open(ustd::StringView path, ub_open_mode_t open_mode = UB_OPEN_MODE_NONE);

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
    ustd::Result<size_t, ub_error_t> ioctl(ub_ioctl_request_t request, void *arg = nullptr);
    ustd::Result<uintptr_t, ub_error_t> mmap();
    ustd::Result<size_t, ub_error_t> read(ustd::Span<void> data);
    ustd::Result<size_t, ub_error_t> read(ustd::Span<void> data, size_t offset);
    ustd::Result<void, ub_error_t> rebind(uint32_t fd);
    ustd::Result<size_t, ub_error_t> size();
    ustd::Result<size_t, ub_error_t> write(ustd::Span<const void> data);

    template <typename T>
    ustd::Result<T *, ub_error_t> mmap();
    template <typename T>
    ustd::Result<T, ub_error_t> read();
    template <typename T>
    ustd::Result<size_t, ub_error_t> write(const T &data);

    uint32_t fd() const override { return *m_fd; }
};

template <typename T>
ustd::Result<T *, ub_error_t> File::mmap() {
    return reinterpret_cast<T *>(TRY(mmap()));
}

template <typename T>
ustd::Result<T, ub_error_t> File::read() {
    T ret{};
    TRY(read({&ret, sizeof(T)}));
    return ret;
}

template <typename T>
ustd::Result<size_t, ub_error_t> File::write(const T &data) {
    return TRY(write({&data, sizeof(T)}));
}

} // namespace core
