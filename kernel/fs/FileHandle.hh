#pragma once

#include <kernel/SysResult.hh>
#include <kernel/SyscallTypes.hh>
#include <kernel/fs/File.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Types.hh>

class VirtSpace;

class FileHandle {
    SharedPtr<File> m_file;
    usize m_offset{0};

public:
    explicit FileHandle(const SharedPtr<File> &file) : m_file(file) {}

    SysResult ioctl(IoctlRequest request, void *arg) const;
    uintptr mmap(VirtSpace &virt_space) const;
    usize read(void *data, usize size);
    usize seek(usize offset, SeekMode mode);
    usize write(void *data, usize size);
    bool valid() const;
};
