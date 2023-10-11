#pragma once

#include <kernel/SpinLock.hh>
#include <kernel/SysResult.hh>
#include <kernel/fs/File.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Span.hh> // IWYU pragma: keep
#include <ustd/StringView.hh>
#include <ustd/Types.hh>

namespace kernel {

enum class InodeType {
    AnonymousFile,
    Directory,
    RegularFile,
};

class Inode {
    Inode *const m_parent;
    const InodeType m_type;
    ustd::SharedPtr<File> m_anonymous_file;
    mutable SpinLock m_anonymous_file_lock;

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

    virtual SysResult<Inode *> child(size_t index) const;
    virtual SysResult<Inode *> create(ustd::StringView name, InodeType type);
    virtual Inode *lookup(ustd::StringView name);
    virtual size_t read(ustd::Span<void> data, size_t offset) const;
    virtual SysResult<> remove(ustd::StringView name);
    virtual size_t size() const = 0;
    virtual SysResult<> truncate();
    virtual size_t write(ustd::Span<const void> data, size_t offset);

    virtual ustd::StringView name() const = 0;
    Inode *parent() const { return m_parent; }
    InodeType type() const { return m_type; }
};

} // namespace kernel
