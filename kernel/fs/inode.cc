#include <kernel/fs/inode.hh>

#include <kernel/error.hh>
#include <kernel/fs/file.hh>
#include <kernel/scoped_lock.hh>
#include <kernel/spin_lock.hh>
#include <kernel/sys_result.hh>
#include <ustd/assert.hh>
#include <ustd/shared_ptr.hh>
#include <ustd/span.hh>
#include <ustd/string_view.hh>
#include <ustd/types.hh>
#include <ustd/utility.hh>

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
