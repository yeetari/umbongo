#pragma once

#include <kernel/SysResult.hh>
#include <kernel/SyscallTypes.hh>
#include <ustd/Shareable.hh>
#include <ustd/Span.hh>
#include <ustd/Types.hh>

namespace kernel {

class VirtSpace;

enum class AttachDirection {
    Read,
    Write,
    ReadWrite,
};

class File : public ustd::Shareable<File> {
public:
    File() = default;
    File(const File &) = delete;
    File(File &&) = delete;
    virtual ~File() = default;

    File &operator=(const File &) = delete;
    File &operator=(File &&) = delete;

    virtual bool is_inode_file() const { return false; }
    virtual bool is_pipe() const { return false; }
    virtual bool is_server_socket() const { return false; }
    virtual bool is_socket() const { return false; }

    virtual void attach(AttachDirection) {}
    virtual void detach(AttachDirection) {}
    virtual bool read_would_block(size_t offset) const = 0;
    virtual bool write_would_block(size_t offset) const = 0;
    virtual SyscallResult ioctl(IoctlRequest, void *) { return SysError::Invalid; }
    virtual uintptr_t mmap(VirtSpace &) const { return 0; }
    virtual SysResult<size_t> read(ustd::Span<void> data, size_t offset = 0) = 0;
    virtual SysResult<size_t> write(ustd::Span<const void> data, size_t offset = 0) = 0;
    virtual bool valid() const { return true; }
};

} // namespace kernel
