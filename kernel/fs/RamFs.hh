#pragma once

#include <kernel/SpinLock.hh>
#include <kernel/fs/File.hh> // IWYU pragma: keep
#include <kernel/fs/FileSystem.hh>
#include <kernel/fs/Inode.hh>
#include <kernel/fs/InodeType.hh>
#include <ustd/Optional.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Span.hh> // IWYU pragma: keep
#include <ustd/String.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>
#include <ustd/Vector.hh>

namespace kernel {

class RamFsInode final : public Inode {
    ustd::String m_name;
    ustd::LargeVector<uint8> m_data;
    ustd::Vector<ustd::UniquePtr<RamFsInode>> m_children;
    mutable SpinLock m_lock;

public:
    RamFsInode(InodeType type, Inode *parent, ustd::StringView name) : Inode(type, parent), m_name(name) {}

    Inode *child(usize index) override;
    Inode *create(ustd::StringView name, InodeType type) override;
    Inode *lookup(ustd::StringView name) override;
    ustd::SharedPtr<File> open_impl() override;
    usize read(ustd::Span<void> data, usize offset) override;
    void remove(ustd::StringView name) override;
    usize size() override;
    void truncate() override;
    usize write(ustd::Span<const void> data, usize offset) override;
    ustd::StringView name() const override { return m_name.view(); }
};

class RamFs final : public FileSystem {
    ustd::Optional<RamFsInode> m_root_inode;

public:
    void mount(Inode *parent, Inode *host) override;
    Inode *root_inode() override { return m_root_inode.obj(); }
};

} // namespace kernel
