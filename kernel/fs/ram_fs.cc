#include <kernel/fs/ram_fs.hh>

#include <kernel/error.hh>
#include <kernel/fs/inode.hh>
#include <kernel/fs/inode_file.hh>
#include <kernel/scoped_lock.hh>
#include <kernel/spin_lock.hh>
#include <kernel/sys_result.hh>
#include <ustd/assert.hh>
#include <ustd/numeric.hh>
#include <ustd/optional.hh>
#include <ustd/shared_ptr.hh>
#include <ustd/span.hh>
#include <ustd/string_view.hh>
#include <ustd/types.hh>
#include <ustd/unique_ptr.hh>
#include <ustd/vector.hh>

namespace kernel {

class File;

void RamFs::mount(Inode *parent, Inode *host) {
    ASSERT(!m_root_inode);
    m_root_inode.emplace(parent, host != nullptr ? host->name() : "/"sv);
}

SysResult<ustd::SharedPtr<File>> RamFsInode::open_impl() {
    return ustd::make_shared<InodeFile>(this);
}

size_t RamFsInode::read(ustd::Span<void> data, size_t offset) const {
    ScopedLock locker(m_lock);
    if (offset >= m_data.size()) {
        return 0;
    }
    size_t size = data.size();
    if (size > m_data.size() - offset) {
        size = m_data.size() - offset;
    }
    __builtin_memcpy(data.data(), m_data.data() + offset, size);
    return size;
}

size_t RamFsInode::size() const {
    ScopedLock locker(m_lock);
    return m_data.size();
}

SysResult<> RamFsInode::truncate() {
    // TODO: Clear with capacity.
    ScopedLock locker(m_lock);
    m_data.clear();
    return {};
}

size_t RamFsInode::write(ustd::Span<const void> data, size_t offset) {
    ScopedLock locker(m_lock);
    size_t size = data.size();
    m_data.ensure_size(offset + size);
    __builtin_memcpy(m_data.data() + offset, data.data(), size);
    return size;
}

SysResult<Inode *> RamFsDirectoryInode::child(size_t index) const {
    if (index >= ustd::Limits<uint32_t>::max()) {
        return Error::Invalid;
    }
    ScopedLock locker(m_lock);
    return m_children[static_cast<uint32_t>(index)].ptr();
}

SysResult<Inode *> RamFsDirectoryInode::create(ustd::StringView name, InodeType type) {
    ScopedLock locker(m_lock);
    switch (type) {
    case InodeType::AnonymousFile:
    case InodeType::RegularFile:
        return m_children.emplace(new RamFsInode(type, this, name)).ptr();
    case InodeType::Directory:
        return m_children.emplace(new RamFsDirectoryInode(this, name)).ptr();
    default:
        return Error::Invalid;
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
            return child.ptr();
        }
    }
    return nullptr;
}

SysResult<> RamFsDirectoryInode::remove(ustd::StringView name) {
    ScopedLock locker(m_lock);
    for (uint32_t i = 0; i < m_children.size(); i++) {
        if (m_children[i]->name() == name) {
            m_children.remove(i);
            return {};
        }
    }
    return Error::NonExistent;
}

size_t RamFsDirectoryInode::size() const {
    ScopedLock locker(m_lock);
    return m_children.size();
}

} // namespace kernel
