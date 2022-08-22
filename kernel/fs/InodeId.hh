#pragma once

#include <ustd/Types.hh>

namespace kernel {

class FileSystem;
class Inode;

class InodeId {
    FileSystem *m_fs{nullptr};
    usize m_index{0};

public:
    InodeId() = default;
    InodeId(FileSystem &fs, usize index) : m_fs(&fs), m_index(index) {}

    Inode *operator->() const;
    bool operator==(const InodeId &) const = default;

    bool is_null() const { return m_fs == nullptr; }
    FileSystem &fs() const { return *m_fs; }
    usize index() const { return m_index; }
};

} // namespace kernel
