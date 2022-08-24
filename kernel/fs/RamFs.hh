#pragma once

#include <kernel/SpinLock.hh>
#include <kernel/SysResult.hh>
#include <kernel/fs/File.hh> // IWYU pragma: keep
#include <kernel/fs/FileSystem.hh>
#include <kernel/fs/Inode.hh>
#include <kernel/fs/InodeType.hh>
#include <ustd/Optional.hh>
#include <ustd/SharedPtr.hh> // IWYU pragma: keep
#include <ustd/Span.hh>      // IWYU pragma: keep
#include <ustd/String.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>
#include <ustd/Vector.hh>

namespace kernel {

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
