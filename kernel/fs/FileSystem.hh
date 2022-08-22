#pragma once

#include <kernel/fs/InodeId.hh>

namespace kernel {

class Inode;

class FileSystem {
public:
    FileSystem() = default;
    FileSystem(const FileSystem &) = delete;
    FileSystem(FileSystem &&) = delete;
    virtual ~FileSystem() = default;

    FileSystem &operator=(const FileSystem &) = delete;
    FileSystem &operator=(FileSystem &&) = delete;

    virtual void mount(const InodeId &parent, const InodeId &host) = 0;
    virtual Inode *inode(const InodeId &id) = 0;
    InodeId root_inode() { return {*this, 0}; }
};

} // namespace kernel
