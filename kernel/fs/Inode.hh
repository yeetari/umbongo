#pragma once

#include <kernel/fs/File.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Span.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>

enum class InodeType {
    Device,
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

    virtual Inode *child(usize index) = 0;
    virtual Inode *create(StringView name, InodeType type) = 0;
    virtual Inode *lookup(StringView name) = 0;
    virtual SharedPtr<File> open() = 0;
    virtual usize read(Span<void> data, usize offset) = 0;
    virtual void remove(StringView name) = 0;
    virtual usize size() = 0;
    virtual void truncate() = 0;
    virtual usize write(Span<const void> data, usize offset) = 0;

    virtual StringView name() const = 0;
    virtual Inode *parent() const = 0;
    InodeType type() const { return m_type; }
};
