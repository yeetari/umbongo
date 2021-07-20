#pragma once

#include <ustd/Optional.hh>
#include <ustd/Span.hh> // IWYU pragma: keep
#include <ustd/StringView.hh>
#include <ustd/Types.hh>

namespace core {

class File {
    Optional<uint32> m_fd;

public:
    explicit File(StringView path);
    File(const File &) = delete;
    File(File &&) = delete;
    ~File();

    File &operator=(const File &) = delete;
    File &operator=(File &&) = delete;

    usize read(Span<void> data);

    explicit operator bool() const { return m_fd.has_value(); }
};

} // namespace core
