#pragma once

#include <kernel/fs/File.hh>
#include <ustd/Span.hh> // IWYU pragma: keep
#include <ustd/Types.hh>

class Inode;

class InodeFile final : public File {
    Inode *const m_inode;

public:
    explicit InodeFile(Inode *inode) : m_inode(inode) {}

    usize read(Span<void> data, usize offset) override;
    usize size() override;
    usize write(Span<const void> data, usize offset) override;
};
