#pragma once

class Inode;

class FileSystem {
public:
    FileSystem() = default;
    FileSystem(const FileSystem &) = delete;
    FileSystem(FileSystem &&) = delete;
    virtual ~FileSystem() = default;

    FileSystem &operator=(const FileSystem &) = delete;
    FileSystem &operator=(FileSystem &&) = delete;

    virtual Inode *root_inode() = 0;
};
