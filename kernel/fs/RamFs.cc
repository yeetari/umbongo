#include <kernel/fs/RamFs.hh>

#include <kernel/fs/File.hh> // IWYU pragma: keep
#include <kernel/fs/Inode.hh>
#include <kernel/fs/InodeFile.hh>
#include <kernel/fs/InodeType.hh>
#include <ustd/Assert.hh>
#include <ustd/Memory.hh>
#include <ustd/Numeric.hh> // IWYU pragma: keep
#include <ustd/Optional.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Span.hh>
#include <ustd/String.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>
#include <ustd/Vector.hh>

// TODO: Needs proper locking.

void RamFs::mount(Inode *parent, Inode *host) {
    ASSERT(!m_root_inode);
    m_root_inode.emplace(InodeType::Directory, parent, host != nullptr ? host->name() : "/"sv);
}

Inode *RamFsInode::child(usize index) {
    ASSERT(index < ustd::Limits<uint32>::max());
    return m_children[static_cast<uint32>(index)].obj();
}

Inode *RamFsInode::create(ustd::StringView name, InodeType type) {
    return m_children.emplace(new RamFsInode(type, this, name)).obj();
}

Inode *RamFsInode::lookup(ustd::StringView name) {
    if (name == ".") {
        return this;
    }
    if (name == "..") {
        return parent();
    }
    for (auto &child : m_children) {
        if (name == child->m_name.view()) {
            return child.obj();
        }
    }
    return nullptr;
}

ustd::SharedPtr<File> RamFsInode::open_impl() {
    return ustd::make_shared<InodeFile>(this);
}

usize RamFsInode::read(ustd::Span<void> data, usize offset) {
    if (offset >= m_data.size()) {
        return 0;
    }
    usize size = data.size();
    if (size > m_data.size() - offset) {
        size = m_data.size() - offset;
    }
    memcpy(data.data(), m_data.data() + offset, size);
    return size;
}

void RamFsInode::remove(ustd::StringView) {
    // TODO: Implement.
    ENSURE_NOT_REACHED();
}

usize RamFsInode::size() {
    return !m_children.empty() ? m_children.size() : m_data.size();
}

void RamFsInode::truncate() {
    ASSERT(m_children.empty());
    m_data.clear();
}

usize RamFsInode::write(ustd::Span<const void> data, usize offset) {
    usize size = data.size();
    m_data.grow(offset + size);
    memcpy(m_data.data() + offset, data.data(), size);
    return size;
}
