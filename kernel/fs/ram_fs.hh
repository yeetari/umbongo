#pragma once

#include <kernel/fs/file_system.hh>
#include <kernel/fs/inode.hh>
#include <kernel/spin_lock.hh>
#include <kernel/sys_result.hh>
#include <ustd/optional.hh>
#include <ustd/shared_ptr.hh> // IWYU pragma: keep
#include <ustd/span.hh>       // IWYU pragma: keep
#include <ustd/string.hh>
#include <ustd/string_view.hh>
#include <ustd/types.hh>
#include <ustd/unique_ptr.hh>
#include <ustd/vector.hh>

namespace kernel {

class File;

class RamFsInode final : public Inode {
    ustd::String m_name;
    ustd::LargeVector<uint8_t> m_data;
    mutable SpinLock m_lock;

public:
    RamFsInode(InodeType type, Inode *parent, ustd::StringView name) : Inode(type, parent), m_name(name) {}

    SysResult<ustd::SharedPtr<File>> open_impl() override;
    size_t read(ustd::Span<void> data, size_t offset) const override;
    size_t size() const override;
    SysResult<> truncate() override;
    size_t write(ustd::Span<const void> data, size_t offset) override;
    ustd::StringView name() const override { return m_name; }
};

class RamFsDirectoryInode final : public Inode {
    ustd::String m_name;
    ustd::Vector<ustd::UniquePtr<Inode>> m_children;
    mutable SpinLock m_lock;

public:
    RamFsDirectoryInode(Inode *parent, ustd::StringView name) : Inode(InodeType::Directory, parent), m_name(name) {}

    SysResult<Inode *> child(size_t index) const override;
    SysResult<Inode *> create(ustd::StringView name, InodeType type) override;
    Inode *lookup(ustd::StringView name) override;
    SysResult<> remove(ustd::StringView name) override;
    size_t size() const override;
    ustd::StringView name() const override { return m_name; }
};

class RamFs final : public FileSystem {
    ustd::Optional<RamFsDirectoryInode> m_root_inode;

public:
    void mount(Inode *parent, Inode *host) override;
    Inode *root_inode() override { return m_root_inode.ptr(); }
};

} // namespace kernel
