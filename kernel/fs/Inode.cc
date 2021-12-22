#include <kernel/fs/Inode.hh>

#include <kernel/SysError.hh>
#include <kernel/SysResult.hh>
#include <kernel/fs/File.hh>
#include <kernel/fs/InodeType.hh>
#include <ustd/Assert.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Utility.hh>

void Inode::bind_anonymous_file(ustd::SharedPtr<File> anonymous_file) {
    ASSERT(!m_anonymous_file);
    m_anonymous_file = ustd::move(anonymous_file);
}

SysResult<ustd::SharedPtr<File>> Inode::open() {
    if (m_type == InodeType::AnonymousFile) {
        if (!m_anonymous_file) {
            return SysError::Invalid;
        }
        return m_anonymous_file;
    }
    return open_impl();
}
