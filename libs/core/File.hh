#pragma once

#include <core/Watchable.hh>
#include <ustd/Optional.hh>
#include <ustd/Span.hh> // IWYU pragma: keep
#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>

namespace core {

class File : public Watchable {
    ustd::Optional<uint32> m_fd;

public:
    explicit File(ustd::Optional<uint32> fd = {}) : m_fd(ustd::move(fd)) {}
    explicit File(ustd::StringView path);
    File(const File &) = delete;
    File(File &&other) noexcept : m_fd(ustd::move(other.m_fd)) {}
    ~File() override;

    File &operator=(const File &) = delete;
    File &operator=(File &&) = delete;

    void close();
    bool open(ustd::StringView path);
    ssize read(ustd::Span<void> data);
    ssize read(ustd::Span<void> data, usize offset);
    ssize rebind(uint32 fd);

    explicit operator bool() const { return m_fd.has_value(); }

    uint32 fd() const override { return *m_fd; }
};

} // namespace core
