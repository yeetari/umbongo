#include <kernel/fs/Inode.hh>

#include <kernel/Error.hh>
#include <kernel/ScopedLock.hh>
#include <kernel/SpinLock.hh>
#include <kernel/SysResult.hh>
#include <kernel/fs/File.hh>
#include <ustd/Assert.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Span.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>

namespace kernel {

SysResult<ustd::SharedPtr<File>> Inode::open_impl() {
    return Error::Invalid;
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
            return Error::Invalid;
        }
        return m_anonymous_file;
    }
    return open_impl();
}

SysResult<Inode *> Inode::child(size_t) const {
    return Error::NotDirectory;
}

SysResult<Inode *> Inode::create(ustd::StringView, InodeType) {
    return Error::NotDirectory;
}

Inode *Inode::lookup(ustd::StringView) {
    ENSURE_NOT_REACHED();
}

size_t Inode::read(ustd::Span<void>, size_t) const {
    ENSURE_NOT_REACHED();
}

SysResult<> Inode::remove(ustd::StringView) {
    return Error::NotDirectory;
}

SysResult<> Inode::truncate() {
    return Error::Invalid;
}

size_t Inode::write(ustd::Span<const void>, size_t) {
    ENSURE_NOT_REACHED();
}

} // namespace kernel
