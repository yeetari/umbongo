#include <kernel/fs/InodeFile.hh>

#include <kernel/SysResult.hh>
#include <kernel/fs/Inode.hh>
#include <ustd/Span.hh>
#include <ustd/Types.hh>

namespace kernel {

SysResult<usize> InodeFile::read(ustd::Span<void> data, usize offset) {
    return m_inode->read(data, offset);
}

SysResult<usize> InodeFile::write(ustd::Span<const void> data, usize offset) {
    return m_inode->write(data, offset);
}

} // namespace kernel
