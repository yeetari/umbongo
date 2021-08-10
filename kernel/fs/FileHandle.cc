#include <kernel/fs/FileHandle.hh>

#include <kernel/SysResult.hh>
#include <kernel/SyscallTypes.hh>
#include <kernel/fs/File.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Types.hh>

SysResult FileHandle::ioctl(IoctlRequest request, void *arg) const {
    return m_file->ioctl(request, arg);
}

uintptr FileHandle::mmap(VirtSpace &virt_space) const {
    return m_file->mmap(virt_space);
}

usize FileHandle::read(void *data, usize size) {
    usize bytes_read = m_file->read({data, size}, m_offset);
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

usize FileHandle::write(void *data, usize size) {
    usize bytes_written = m_file->write({data, size}, m_offset);
    m_offset += bytes_written;
    return bytes_written;
}

bool FileHandle::valid() const {
    return m_file->valid();
}
