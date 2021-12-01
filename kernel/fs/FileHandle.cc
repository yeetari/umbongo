#include <kernel/fs/FileHandle.hh>

#include <kernel/SysResult.hh>
#include <kernel/SyscallTypes.hh>
#include <kernel/fs/File.hh>
#include <ustd/Result.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Types.hh>

FileHandle::FileHandle(const ustd::SharedPtr<File> &file, AttachDirection direction)
    : m_file(file), m_direction(direction) {
    file->attach(direction);
}

FileHandle::~FileHandle() {
    if (m_file) {
        m_file->detach(m_direction);
    }
}

SyscallResult FileHandle::ioctl(IoctlRequest request, void *arg) const {
    return m_file->ioctl(request, arg);
}

uintptr FileHandle::mmap(VirtSpace &virt_space) const {
    return m_file->mmap(virt_space);
}

SysResult<usize> FileHandle::read(void *data, usize size) {
    auto bytes_read = m_file->read({data, size}, m_offset);
    if (bytes_read.is_error()) {
        return bytes_read.error();
    }
    m_offset += *bytes_read;
    return *bytes_read;
}

usize FileHandle::seek(usize offset, SeekMode mode) {
    switch (mode) {
    case SeekMode::Add:
        m_offset += offset;
        break;
    case SeekMode::Set:
        m_offset = offset;
        break;
    }
    return m_offset;
}

SysResult<usize> FileHandle::write(void *data, usize size) {
    auto bytes_written = m_file->write({data, size}, m_offset);
    if (bytes_written.is_error()) {
        return bytes_written.error();
    }
    m_offset += *bytes_written;
    return *bytes_written;
}

bool FileHandle::valid() const {
    return m_file->valid();
}
