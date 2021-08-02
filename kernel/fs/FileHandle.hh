#pragma once

#include <kernel/fs/File.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Types.hh>

class VirtSpace;

class FileHandle {
    SharedPtr<File> m_file;
    uint64 m_offset{0};

public:
    explicit FileHandle(const SharedPtr<File> &file) : m_file(file) {}

    uintptr mmap(VirtSpace &virt_space) const;
    usize read(void *data, usize size) const;
    void seek(uint64 offset);
    usize write(void *data, usize size) const;
    bool valid() const;
};
