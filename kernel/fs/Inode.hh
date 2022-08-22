#pragma once

#include <kernel/SpinLock.hh>
#include <kernel/SysResult.hh>
#include <kernel/fs/File.hh>
#include <kernel/fs/InodeId.hh>
#include <kernel/fs/InodeType.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Span.hh> // IWYU pragma: keep
#include <ustd/StringView.hh>
#include <ustd/Types.hh>

namespace kernel {

class Inode {
    const InodeId m_id;
    const InodeId m_parent;
    const InodeType m_type;
    ustd::SharedPtr<File> m_anonymous_file;
    mutable SpinLock m_anonymous_file_lock;

public:
    Inode(InodeId id, InodeId parent, InodeType type) : m_id(id), m_parent(parent), m_type(type) {}
    Inode(const Inode &) = delete;
    Inode(Inode &&) = delete;
    virtual ~Inode() = default;

    Inode &operator=(const Inode &) = delete;
    Inode &operator=(Inode &&) = delete;

    void bind_anonymous_file(ustd::SharedPtr<File> anonymous_file);
    SysResult<ustd::SharedPtr<File>> open();

    virtual SysResult<InodeId> child(usize index) const;
    virtual SysResult<InodeId> create(ustd::StringView name, InodeType type);
    virtual InodeId lookup(ustd::StringView name);
    virtual usize read(ustd::Span<void> data, usize offset) const;
    virtual SysResult<> remove(ustd::StringView name);
    virtual usize size() const = 0;
    virtual SysResult<> truncate();
    virtual usize write(ustd::Span<const void> data, usize offset);

    virtual ustd::StringView name() const = 0;
    const InodeId &id() const { return m_id; }
    const InodeId &parent() const { return m_parent; }
    InodeType type() const { return m_type; }
};

} // namespace kernel
