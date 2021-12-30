#include <kernel/fs/RamFs.hh>

#include <kernel/ScopedLock.hh> // IWYU pragma: keep
#include <kernel/SysError.hh>
#include <kernel/SysResult.hh>
#include <kernel/fs/File.hh> // IWYU pragma: keep
#include <kernel/fs/Inode.hh>
#include <kernel/fs/InodeFile.hh>
#include <kernel/fs/InodeType.hh>
#include <ustd/Assert.hh>
#include <ustd/Numeric.hh> // IWYU pragma: keep
#include <ustd/Optional.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Span.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>
#include <ustd/Vector.hh>

namespace kernel {

void RamFs::mount(Inode *parent, Inode *host) {
    ASSERT(!m_root_inode);
    m_root_inode.emplace(parent, host != nullptr ? host->name() : "/"sv);
}

SysResult<ustd::SharedPtr<File>> RamFsInode::open_impl() {
    return ustd::make_shared<InodeFile>(this);
}

usize RamFsInode::read(ustd::Span<void> data, usize offset) const {
    ScopedLock locker(m_lock);
    if (offset >= m_data.size()) {
        return 0;
    }
    usize size = data.size();
    if (size > m_data.size() - offset) {
        size = m_data.size() - offset;
    }
    __builtin_memcpy(data.data(), m_data.data() + offset, size);
    return size;
}

usize RamFsInode::size() const {
    ScopedLock locker(m_lock);
    return m_data.size();
}

SysResult<> RamFsInode::truncate() {
    // TODO: Clear with capacity.
    ScopedLock locker(m_lock);
    m_data.clear();
    return {};
}

usize RamFsInode::write(ustd::Span<const void> data, usize offset) {
    ScopedLock locker(m_lock);
    usize size = data.size();
    m_data.ensure_size(offset + size);
    __builtin_memcpy(m_data.data() + offset, data.data(), size);
    return size;
}

SysResult<Inode *> RamFsDirectoryInode::child(usize index) const {
    if (index >= ustd::Limits<uint32>::max()) {
        return SysError::Invalid;
    }
    ScopedLock locker(m_lock);
    return m_children[static_cast<uint32>(index)].obj();
}

SysResult<Inode *> RamFsDirectoryInode::create(ustd::StringView name, InodeType type) {
    ScopedLock locker(m_lock);
    switch (type) {
    case InodeType::AnonymousFile:
    case InodeType::RegularFile:
        return m_children.emplace(new RamFsInode(type, this, name)).obj();
    case InodeType::Directory:
        return m_children.emplace(new RamFsDirectoryInode(this, name)).obj();
    default:
        return SysError::Invalid;
    }
}

Inode *RamFsDirectoryInode::lookup(ustd::StringView name) {
    if (name == ".") {
        return this;
    }
    if (name == "..") {
        return parent();
    }
    ScopedLock locker(m_lock);
    for (const auto &child : m_children) {
        if (child->name() == name) {
            return child.obj();
        }
    }
    return nullptr;
}

SysResult<> RamFsDirectoryInode::remove(ustd::StringView name) {
    ScopedLock locker(m_lock);
    for (uint32 i = 0; i < m_children.size(); i++) {
        if (m_children[i]->name() == name) {
            m_children.remove(i);
            return {};
        }
    }
    return SysError::NonExistent;
}

usize RamFsDirectoryInode::size() const {
    ScopedLock locker(m_lock);
    return m_children.size();
}

} // namespace kernel
