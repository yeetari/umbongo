#pragma once

#include <kernel/SysResult.hh>
#include <kernel/fs/File.hh>
#include <ustd/Span.hh> // IWYU pragma: keep
#include <ustd/Types.hh>

namespace kernel {

class Inode;

class InodeFile final : public File {
    Inode *const m_inode;

public:
    explicit InodeFile(Inode *inode) : m_inode(inode) {}

    bool is_inode_file() const override { return true; }

    bool read_would_block(size_t) const override { return false; }
    bool write_would_block(size_t) const override { return false; }
    SysResult<size_t> read(ustd::Span<void> data, size_t offset) override;
    SysResult<size_t> write(ustd::Span<const void> data, size_t offset) override;

    Inode *inode() const { return m_inode; }
};

} // namespace kernel
