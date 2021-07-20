#pragma once

#include <ustd/Span.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>

enum InodeType {
    Directory,
    RegularFile,
};

class Inode {
    InodeType m_type;

public:
    explicit Inode(InodeType type) : m_type(type) {}
    Inode(const Inode &) = delete;
    Inode(Inode &&) noexcept = default;
    virtual ~Inode() = default;

    Inode &operator=(const Inode &) = delete;
    Inode &operator=(Inode &&) noexcept = default;

    virtual Inode *create(StringView name, InodeType type) = 0;
    virtual Inode *lookup(StringView name) = 0;
    virtual usize read(Span<void> data, usize offset) = 0;
    virtual usize size() = 0;
    virtual usize write(Span<const void> data, usize offset) = 0;
};