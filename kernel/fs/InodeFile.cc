#include <kernel/fs/InodeFile.hh>

#include <kernel/SysResult.hh>
#include <kernel/fs/Inode.hh>
#include <ustd/Span.hh>
#include <ustd/Types.hh>

SysResult<usize> InodeFile::read(Span<void> data, usize offset) {
    return m_inode->read(data, offset);
}

SysResult<usize> InodeFile::write(Span<const void> data, usize offset) {
    return m_inode->write(data, offset);
}
