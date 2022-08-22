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
    ustd::LargeVector<uint8> m_data;
    mutable SpinLock m_lock;

public:
    RamFsInode(const InodeId &id, const InodeId &parent, InodeType type, ustd::StringView name)
        : Inode(id, parent, type), m_name(name) {}

    usize read(ustd::Span<void> data, usize offset) const override;
    usize size() const override;
    SysResult<> truncate() override;
    usize write(ustd::Span<const void> data, usize offset) override;
    ustd::StringView name() const override { return m_name; }
};

class RamFsDirectoryInode final : public Inode {
    ustd::String m_name;
    ustd::Vector<InodeId> m_children;
    mutable SpinLock m_lock;

public:
    RamFsDirectoryInode(const InodeId &id, const InodeId &parent, ustd::StringView name)
        : Inode(id, parent, InodeType::Directory), m_name(name) {}

    SysResult<InodeId> child(usize index) const override;
    SysResult<InodeId> create(ustd::StringView name, InodeType type) override;
    InodeId lookup(ustd::StringView name) override;
    SysResult<> remove(ustd::StringView name) override;
    usize size() const override;
    ustd::StringView name() const override { return m_name; }
};

class RamFs final : public FileSystem {
    friend RamFsDirectoryInode;

private:
    ustd::LargeVector<ustd::UniquePtr<Inode>> m_inodes;

public:
    void mount(const InodeId &parent, const InodeId &host) override;
    Inode *inode(const InodeId &id) override;
};

} // namespace kernel
