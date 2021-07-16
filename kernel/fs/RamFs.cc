#include <kernel/fs/RamFs.hh>

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
