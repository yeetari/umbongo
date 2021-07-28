#include <kernel/fs/RamFs.hh>

#include <kernel/fs/File.hh> // IWYU pragma: keep
#include <kernel/fs/Inode.hh>
#include <kernel/fs/InodeFile.hh>
#include <ustd/Memory.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Span.hh>
#include <ustd/String.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

Inode *RamFsInode::create(StringView name, InodeType type) {
    return &m_children.emplace(type, name, this);
}

Inode *RamFsInode::lookup(StringView name) {
    if (name == ".") {
        return this;
    }
    if (name == "..") {
        return m_parent;
    }
    for (auto &child : m_children) {
        if (name == child.m_name.view()) {
            return &child;
        }
    }
    return nullptr;
}

SharedPtr<File> RamFsInode::open() {
    return ustd::make_shared<InodeFile>(this);
}

usize RamFsInode::read(Span<void> data, usize offset) {
    // TODO: Verify offset in range.
    usize size = data.size();
    if (size > m_data.size()) {
        size = m_data.size();
    }
    memcpy(data.data(), m_data.data() + offset, size);
    return size;
}

usize RamFsInode::size() {
    return m_data.size();
}

usize RamFsInode::write(Span<const void> data, usize offset) {
    usize size = data.size();
    m_data.resize(offset + size);
    memcpy(m_data.data() + offset, data.data(), size);
    return size;
}
