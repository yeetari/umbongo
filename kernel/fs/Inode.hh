#pragma once

#include <kernel/SysResult.hh>
#include <kernel/fs/File.hh>
#include <kernel/fs/InodeType.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Span.hh> // IWYU pragma: keep
#include <ustd/StringView.hh>
#include <ustd/Types.hh>

namespace kernel {

class Inode {
    Inode *const m_parent;
    const InodeType m_type;
    ustd::SharedPtr<File> m_anonymous_file;

protected:
    virtual SysResult<ustd::SharedPtr<File>> open_impl();

public:
    Inode(InodeType type, Inode *parent) : m_parent(parent), m_type(type) {}
    Inode(const Inode &) = delete;
    Inode(Inode &&) = delete;
    virtual ~Inode() = default;

    Inode &operator=(const Inode &) = delete;
    Inode &operator=(Inode &&) = delete;

    void bind_anonymous_file(ustd::SharedPtr<File> anonymous_file);
    SysResult<ustd::SharedPtr<File>> open();

    virtual SysResult<Inode *> child(usize index) const;
    virtual SysResult<Inode *> create(ustd::StringView name, InodeType type);
    virtual Inode *lookup(ustd::StringView name);
    virtual usize read(ustd::Span<void> data, usize offset) const;
    virtual SysResult<> remove(ustd::StringView name);
    virtual usize size() const = 0;
    virtual SysResult<> truncate();
    virtual usize write(ustd::Span<const void> data, usize offset);

    virtual ustd::StringView name() const = 0;
    Inode *parent() const { return m_parent; }
    InodeType type() const { return m_type; }
};

} // namespace kernel
