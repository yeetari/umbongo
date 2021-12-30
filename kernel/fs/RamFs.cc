#include <kernel/fs/RamFs.hh>

#include <kernel/ScopedLock.hh> // IWYU pragma: keep
#include <kernel/fs/File.hh>    // IWYU pragma: keep
#include <kernel/fs/Inode.hh>
#include <kernel/fs/InodeFile.hh>
#include <kernel/fs/InodeType.hh>
#include <ustd/Assert.hh>
#include <ustd/Numeric.hh> // IWYU pragma: keep
#include <ustd/Optional.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Span.hh>
#include <ustd/String.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>
#include <ustd/Vector.hh>

namespace kernel {

void RamFs::mount(Inode *parent, Inode *host) {
    ASSERT(!m_root_inode);
    m_root_inode.emplace(InodeType::Directory, parent, host != nullptr ? host->name() : "/"sv);
}

Inode *RamFsInode::child(usize index) {
    ASSERT(index < ustd::Limits<uint32>::max());
    ScopedLock locker(m_lock);
    return m_children[static_cast<uint32>(index)].obj();
}

Inode *RamFsInode::create(ustd::StringView name, InodeType type) {
    ScopedLock locker(m_lock);
    return m_children.emplace(new RamFsInode(type, this, name)).obj();
}

Inode *RamFsInode::lookup(ustd::StringView name) {
    if (name == ".") {
        return this;
    }
    if (name == "..") {
        return parent();
    }
    ScopedLock locker(m_lock);
    for (auto &child : m_children) {
        if (name == child->m_name) {
            return child.obj();
        }
    }
    return nullptr;
}

ustd::SharedPtr<File> RamFsInode::open_impl() {
    return ustd::make_shared<InodeFile>(this);
}

usize RamFsInode::read(ustd::Span<void> data, usize offset) {
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

void RamFsInode::remove(ustd::StringView) {
    // TODO: Implement.
    ENSURE_NOT_REACHED();
}

usize RamFsInode::size() {
    ScopedLock locker(m_lock);
    return !m_children.empty() ? m_children.size() : m_data.size();
}

void RamFsInode::truncate() {
    // TODO: Clear with capacity.
    ScopedLock locker(m_lock);
    ASSERT(m_children.empty());
    m_data.clear();
}

usize RamFsInode::write(ustd::Span<const void> data, usize offset) {
    ScopedLock locker(m_lock);
    usize size = data.size();
    m_data.ensure_size(offset + size);
    __builtin_memcpy(m_data.data() + offset, data.data(), size);
    return size;
}

} // namespace kernel
