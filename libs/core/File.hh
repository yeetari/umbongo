#pragma once

#include <core/Watchable.hh>
#include <kernel/SysError.hh>
#include <ustd/Optional.hh>
#include <ustd/Result.hh>
#include <ustd/Span.hh> // IWYU pragma: keep
#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>

namespace core {

class File : public Watchable {
    ustd::Optional<uint32> m_fd;

public:
    static ustd::Result<File, SysError> open(ustd::StringView path);

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
    ssize read(ustd::Span<void> data);
    ssize read(ustd::Span<void> data, usize offset);
    template <typename T>
    ustd::Result<T, SysError> read();
    ssize rebind(uint32 fd);

    uint32 fd() const override { return *m_fd; }
};

template <typename T>
ustd::Result<T, SysError> File::read() {
    T ret{};
    if (auto rc = read({&ret, sizeof(T)}); rc != sizeof(T)) {
        return static_cast<SysError>(rc);
    }
    return ret;
}

} // namespace core
