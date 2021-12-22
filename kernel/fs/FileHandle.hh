#pragma once

#include <kernel/SysResult.hh>
#include <kernel/SyscallTypes.hh>
#include <kernel/fs/File.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Types.hh>

namespace kernel {

class VirtSpace;

class FileHandle {
    ustd::SharedPtr<File> m_file;
    AttachDirection m_direction;
    usize m_offset{0};

public:
    explicit FileHandle(const ustd::SharedPtr<File> &file, AttachDirection direction = AttachDirection::ReadWrite);
    FileHandle(const FileHandle &other) : FileHandle(other.m_file, other.m_direction) { m_offset = other.m_offset; }
    FileHandle(FileHandle &&) = default;
    ~FileHandle();

    FileHandle &operator=(const FileHandle &) = delete;
    FileHandle &operator=(FileHandle &&) = delete;

    SyscallResult ioctl(IoctlRequest request, void *arg) const;
    uintptr mmap(VirtSpace &virt_space) const;
    SysResult<usize> read(void *data, usize size);
    usize seek(usize offset, SeekMode mode);
    SysResult<usize> write(void *data, usize size);
    bool valid() const;

    File &file() const { return *m_file; }
};

} // namespace kernel
