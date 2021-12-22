#pragma once

#include <kernel/SysResult.hh>
#include <kernel/fs/File.hh>
#include <kernel/fs/InodeType.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Span.hh> // IWYU pragma: keep
#include <ustd/StringView.hh>
#include <ustd/Types.hh>

class Inode {
    Inode *const m_parent;
    const InodeType m_type;
    ustd::SharedPtr<File> m_anonymous_file;

protected:
    virtual ustd::SharedPtr<File> open_impl() = 0;

public:
    Inode(InodeType type, Inode *parent) : m_parent(parent), m_type(type) {}
    Inode(const Inode &) = delete;
    Inode(Inode &&) = delete;
    virtual ~Inode() = default;

    Inode &operator=(const Inode &) = delete;
    Inode &operator=(Inode &&) = delete;

    void bind_anonymous_file(ustd::SharedPtr<File> anonymous_file);
    SysResult<ustd::SharedPtr<File>> open();

    virtual Inode *child(usize index) = 0;
    virtual Inode *create(ustd::StringView name, InodeType type) = 0;
    virtual Inode *lookup(ustd::StringView name) = 0;
    virtual usize read(ustd::Span<void> data, usize offset) = 0;
    virtual void remove(ustd::StringView name) = 0;
    virtual usize size() = 0;
    virtual void truncate() = 0;
    virtual usize write(ustd::Span<const void> data, usize offset) = 0;

    virtual ustd::StringView name() const = 0;
    Inode *parent() const { return m_parent; }
    InodeType type() const { return m_type; }
};
