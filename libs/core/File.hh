#pragma once

#include <core/Watchable.hh>
#include <ustd/Optional.hh>
#include <ustd/Span.hh> // IWYU pragma: keep
#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>

namespace core {

class File : public Watchable {
    Optional<uint32> m_fd;

public:
    explicit File(Optional<uint32> fd = {}) : m_fd(ustd::move(fd)) {}
    explicit File(StringView path);
    File(const File &) = delete;
    File(File &&other) noexcept : m_fd(ustd::move(other.m_fd)) {}
    ~File() override;

    File &operator=(const File &) = delete;
    File &operator=(File &&) = delete;

    void close();
    bool open(StringView path);
    ssize read(Span<void> data);
    ssize read(Span<void> data, usize offset);
    ssize rebind(uint32 fd);

    explicit operator bool() const { return m_fd.has_value(); }

    uint32 fd() const override { return *m_fd; }
};

} // namespace core
