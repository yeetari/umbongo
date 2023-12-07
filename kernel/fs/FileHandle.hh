#pragma once

#include <kernel/SysResult.hh>
#include <kernel/api/Types.h>
#include <kernel/fs/File.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Types.hh>

namespace kernel {

class VirtSpace;

class FileHandle {
    ustd::SharedPtr<File> m_file;
    AttachDirection m_direction;
    size_t m_offset{0};

public:
    explicit FileHandle(const ustd::SharedPtr<File> &file, AttachDirection direction = AttachDirection::ReadWrite);
    FileHandle(const FileHandle &other) : FileHandle(other.m_file, other.m_direction) { m_offset = other.m_offset; }
    FileHandle(FileHandle &&) = default;
    ~FileHandle();

    FileHandle &operator=(const FileHandle &) = delete;
    FileHandle &operator=(FileHandle &&) = delete;

    bool read_would_block() const;
    bool write_would_block() const;
    SyscallResult ioctl(ub_ioctl_request_t request, void *arg) const;
    uintptr_t mmap(VirtSpace &virt_space) const;
    SysResult<size_t> read(void *data, size_t size);
    size_t seek(size_t offset, ub_seek_mode_t mode);
    SysResult<size_t> write(void *data, size_t size);
    bool valid() const;

    File &file() const { return *m_file; }
    size_t offset() const { return m_offset; }
};

} // namespace kernel
