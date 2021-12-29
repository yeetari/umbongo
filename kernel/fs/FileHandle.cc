#include <kernel/fs/FileHandle.hh>

#include <kernel/SysResult.hh>
#include <kernel/SyscallTypes.hh>
#include <kernel/fs/File.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Try.hh>
#include <ustd/Types.hh>

namespace kernel {

FileHandle::FileHandle(const ustd::SharedPtr<File> &file, AttachDirection direction)
    : m_file(file), m_direction(direction) {
    file->attach(direction);
}

FileHandle::~FileHandle() {
    if (m_file) {
        m_file->detach(m_direction);
    }
}

bool FileHandle::read_would_block() const {
    return m_file->read_would_block(m_offset);
}

bool FileHandle::write_would_block() const {
    return m_file->write_would_block(m_offset);
}

SyscallResult FileHandle::ioctl(IoctlRequest request, void *arg) const {
    return m_file->ioctl(request, arg);
}

uintptr FileHandle::mmap(VirtSpace &virt_space) const {
    return m_file->mmap(virt_space);
}

SysResult<usize> FileHandle::read(void *data, usize size) {
    usize bytes_read = TRY(m_file->read({data, size}, m_offset));
    m_offset += bytes_read;
    return bytes_read;
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
    usize bytes_written = TRY(m_file->write({data, size}, m_offset));
    m_offset += bytes_written;
    return bytes_written;
}

bool FileHandle::valid() const {
    return m_file->valid();
}

} // namespace kernel
