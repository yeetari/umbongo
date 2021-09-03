#pragma once

#include <kernel/fs/File.hh>
#include <kernel/fs/InodeType.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Span.hh> // IWYU pragma: keep
#include <ustd/StringView.hh>
#include <ustd/Types.hh>

class Inode {
    Inode *m_parent;
    InodeType m_type;
    SharedPtr<File> m_ipc_file;

protected:
    virtual SharedPtr<File> open_impl() = 0;

public:
    Inode(InodeType type, Inode *parent) : m_parent(parent), m_type(type) {}
    Inode(const Inode &) = delete;
    Inode(Inode &&) noexcept = default;
    virtual ~Inode() = default;

    Inode &operator=(const Inode &) = delete;
    Inode &operator=(Inode &&) noexcept = default;

    void bind_ipc_file(SharedPtr<File> ipc_file);
    SharedPtr<File> open();

    virtual Inode *child(usize index) = 0;
    virtual Inode *create(StringView name, InodeType type) = 0;
    virtual Inode *lookup(StringView name) = 0;
    virtual usize read(Span<void> data, usize offset) = 0;
    virtual void remove(StringView name) = 0;
    virtual usize size() = 0;
    virtual void truncate() = 0;
    virtual usize write(Span<const void> data, usize offset) = 0;

    virtual StringView name() const = 0;
    Inode *parent() const { return m_parent; }
    InodeType type() const { return m_type; }
    SharedPtr<File> ipc_file() const { return m_ipc_file; }
};
