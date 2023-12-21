#include <kernel/fs/inode_file.hh>

#include <kernel/fs/inode.hh>
#include <kernel/sys_result.hh>
#include <ustd/span.hh>
#include <ustd/types.hh>

namespace kernel {

SysResult<size_t> InodeFile::read(ustd::Span<void> data, size_t offset) {
    return m_inode->read(data, offset);
}

SysResult<size_t> InodeFile::write(ustd::Span<const void> data, size_t offset) {
    return m_inode->write(data, offset);
}

} // namespace kernel
