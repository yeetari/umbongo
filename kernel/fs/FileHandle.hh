#pragma once

#include <kernel/fs/File.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Types.hh>

class FileHandle {
    SharedPtr<File> m_file;

public:
    explicit FileHandle(const SharedPtr<File> &file) : m_file(file) {}

    uint64 read(void *data, usize size) const;
    uint64 write(void *data, usize size) const;
};
