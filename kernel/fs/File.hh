#pragma once

#include <kernel/SysResult.hh>
#include <kernel/SyscallTypes.hh>
#include <ustd/Shareable.hh>
#include <ustd/Span.hh>
#include <ustd/Types.hh>

class VirtSpace;

enum class AttachDirection {
    Read,
    Write,
    ReadWrite,
};

class File : public Shareable<File> {
public:
    File() = default;
    File(const File &) = delete;
    File(File &&) = delete;
    virtual ~File() = default;

    File &operator=(const File &) = delete;
    File &operator=(File &&) = delete;

    // TODO: Const some of these?
    virtual void attach(AttachDirection) {}
    virtual void detach(AttachDirection) {}
    virtual bool can_read() = 0;
    virtual bool can_write() = 0;
    virtual SyscallResult ioctl(IoctlRequest, void *) { return SysError::Invalid; }
    virtual uintptr mmap(VirtSpace &) { return 0; }
    virtual usize read(Span<void> data, usize offset = 0) = 0;
    virtual usize size() = 0;
    virtual usize write(Span<const void> data, usize offset = 0) = 0;
    virtual bool valid() { return true; }
};
