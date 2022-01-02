#include <kernel/fs/Inode.hh>

#include <kernel/ScopedLock.hh>
#include <kernel/SysError.hh>
#include <kernel/SysResult.hh>
#include <kernel/fs/File.hh>
#include <kernel/fs/InodeType.hh>
#include <ustd/Assert.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Span.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>

namespace kernel {

SysResult<ustd::SharedPtr<File>> Inode::open_impl() {
    return SysError::Invalid;
}

void Inode::bind_anonymous_file(ustd::SharedPtr<File> anonymous_file) {
    ScopedLock locker(m_anonymous_file_lock);
    ASSERT(!m_anonymous_file);
    m_anonymous_file = ustd::move(anonymous_file);
}

SysResult<ustd::SharedPtr<File>> Inode::open() {
    if (m_type == InodeType::AnonymousFile) {
        ScopedLock locker(m_anonymous_file_lock);
        if (!m_anonymous_file) {
            return SysError::Invalid;
        }
        return m_anonymous_file;
    }
    return open_impl();
}

SysResult<Inode *> Inode::child(usize) const {
    return SysError::NotDirectory;
}

SysResult<Inode *> Inode::create(ustd::StringView, InodeType) {
    return SysError::NotDirectory;
}

Inode *Inode::lookup(ustd::StringView) {
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
