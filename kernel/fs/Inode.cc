#include <kernel/fs/Inode.hh>

#include <kernel/ScopedLock.hh>
#include <kernel/SysError.hh>
#include <kernel/SysResult.hh>
#include <kernel/fs/File.hh>
#include <kernel/fs/FileSystem.hh>
#include <kernel/fs/InodeFile.hh>
#include <kernel/fs/InodeType.hh>
#include <ustd/Assert.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Span.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>

namespace kernel {

Inode *InodeId::operator->() const {
    return m_fs->inode(*this);
}

void Inode::bind_anonymous_file(ustd::SharedPtr<File> anonymous_file) {
    ScopedLock locker(m_anonymous_file_lock);
    ASSERT(!m_anonymous_file);
    m_anonymous_file = ustd::move(anonymous_file);
}

SysResult<ustd::SharedPtr<File>> Inode::open() {
    ScopedLock locker(m_anonymous_file_lock);
    if (m_anonymous_file) {
        return m_anonymous_file;
    }
    if (m_type == InodeType::AnonymousFile) {
        return SysError::Invalid;
    }
    locker.unlock();
    if (m_type == InodeType::RegularFile) {
        return ustd::make_shared<InodeFile>(m_id);
    }
    return SysError::Invalid;
}

SysResult<InodeId> Inode::child(usize) const {
    return SysError::NotDirectory;
}

SysResult<InodeId> Inode::create(ustd::StringView, InodeType) {
    return SysError::NotDirectory;
}

InodeId Inode::lookup(ustd::StringView) {
    ENSURE_NOT_REACHED();
}

usize Inode::read(ustd::Span<void>, usize) const {
    ENSURE_NOT_REACHED();
}

SysResult<> Inode::remove(ustd::StringView) {
    return SysError::NotDirectory;
}

SysResult<> Inode::truncate() {
    return SysError::Invalid;
}

usize Inode::write(ustd::Span<const void>, usize) {
    ENSURE_NOT_REACHED();
}

} // namespace kernel
