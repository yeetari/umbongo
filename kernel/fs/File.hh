#pragma once

#include <ustd/Shareable.hh>
#include <ustd/Span.hh>
#include <ustd/Types.hh>

class File : public Shareable<File> {
public:
    File() = default;
    File(const File &) = delete;
    File(File &&) = delete;
    virtual ~File() = default;

    File &operator=(const File &) = delete;
    File &operator=(File &&) = delete;

    virtual usize read(Span<void> data, usize offset = 0) = 0;
    virtual usize size() = 0;
    virtual usize write(Span<const void> data, usize offset = 0) = 0;
    virtual bool valid() { return true; }
};
