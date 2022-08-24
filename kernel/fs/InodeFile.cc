#include <kernel/fs/InodeFile.hh>

#include <kernel/SysResult.hh>
#include <kernel/fs/Inode.hh>
#include <ustd/Span.hh>
#include <ustd/Types.hh>

namespace kernel {

SysResult<size_t> InodeFile::read(ustd::Span<void> data, size_t offset) {
    return m_inode->read(data, offset);
}

SysResult<size_t> InodeFile::write(ustd::Span<const void> data, size_t offset) {
    return m_inode->write(data, offset);
}

} // namespace kernel
