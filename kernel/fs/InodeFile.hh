#pragma once

#include <kernel/SysResult.hh>
#include <kernel/fs/File.hh>
#include <ustd/Span.hh> // IWYU pragma: keep
#include <ustd/Types.hh>

class Inode;

class InodeFile final : public File {
    Inode *const m_inode;

public:
    explicit InodeFile(Inode *inode) : m_inode(inode) {}

    bool is_inode_file() const override { return true; }

    bool can_read() override { return true; }
    bool can_write() override { return true; }
    SysResult<usize> read(Span<void> data, usize offset) override;
    SysResult<usize> write(Span<const void> data, usize offset) override;

    Inode *inode() const { return m_inode; }
};
