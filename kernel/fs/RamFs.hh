#pragma once

#include <kernel/fs/FileSystem.hh>
#include <kernel/fs/Inode.hh>
#include <ustd/String.hh>
#include <ustd/Vector.hh>

class RamFsInode final : public Inode {
    String m_name;
    LargeVector<uint8> m_data;

    RamFsInode *m_parent;
    Vector<RamFsInode> m_children;

public:
    RamFsInode(InodeType type, StringView name, RamFsInode *parent) : Inode(type), m_name(name), m_parent(parent) {}

    Inode *create(StringView name, InodeType type) override;
    Inode *lookup(StringView name) override;
    usize read(Span<void> data, usize offset) override;
    usize size() override;
    usize write(Span<const void> data, usize offset) override;
};

class RamFs final : public FileSystem {
    RamFsInode m_root_inode;

public:
    RamFs() : m_root_inode(InodeType::Directory, "/", &m_root_inode) {}

    Inode *root_inode() override { return &m_root_inode; }
};
