#include <kernel/fs/InodeFile.hh>

#include <kernel/fs/Inode.hh>
#include <ustd/Span.hh>
#include <ustd/Types.hh>

usize InodeFile::read(Span<void> data, usize offset) {
    return m_inode->read(data, offset);
}

usize InodeFile::size() {
    return m_inode->size();
}

usize InodeFile::write(Span<const void> data, usize offset) {
    return m_inode->write(data, offset);
}
