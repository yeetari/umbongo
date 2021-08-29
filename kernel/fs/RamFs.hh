#pragma once

#include <kernel/fs/File.hh> // IWYU pragma: keep
#include <kernel/fs/FileSystem.hh>
#include <kernel/fs/Inode.hh>
#include <ustd/Optional.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Span.hh> // IWYU pragma: keep
#include <ustd/String.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>
#include <ustd/Vector.hh>

class RamFsInode final : public Inode {
    String m_name;
    LargeVector<uint8> m_data;
    Vector<UniquePtr<RamFsInode>> m_children;

public:
    RamFsInode(InodeType type, Inode *parent, StringView name) : Inode(type, parent), m_name(name) {}

    Inode *child(usize index) override;
    Inode *create(StringView name, InodeType type) override;
    Inode *lookup(StringView name) override;
    SharedPtr<File> open() override;
    usize read(Span<void> data, usize offset) override;
    void remove(StringView name) override;
    usize size() override;
    void truncate() override;
    usize write(Span<const void> data, usize offset) override;
    StringView name() const override { return m_name.view(); }
};

class RamFs final : public FileSystem {
    Optional<RamFsInode> m_root_inode;

public:
    void mount(Inode *parent, Inode *host) override;
    Inode *root_inode() override { return m_root_inode.obj(); }
};
