#pragma once

#include <kernel/SysResult.hh>
#include <kernel/fs/File.hh>
#include <kernel/fs/InodeId.hh>
#include <ustd/Span.hh> // IWYU pragma: keep
#include <ustd/Types.hh>

namespace kernel {

class InodeFile final : public File {
    const InodeId m_inode;

public:
    explicit InodeFile(const InodeId &inode) : m_inode(inode) {}

    bool is_inode_file() const override { return true; }

    bool read_would_block(usize) const override { return false; }
    bool write_would_block(usize) const override { return false; }
    SysResult<usize> read(ustd::Span<void> data, usize offset) override;
    SysResult<usize> write(ustd::Span<const void> data, usize offset) override;

    const InodeId &inode() const { return m_inode; }
};

} // namespace kernel
