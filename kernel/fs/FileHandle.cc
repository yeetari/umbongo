#include <kernel/fs/FileHandle.hh>

#include <kernel/fs/File.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Types.hh>

uintptr FileHandle::mmap(VirtSpace &virt_space) const {
    return m_file->mmap(virt_space);
}

usize FileHandle::read(void *data, usize size) const {
    return m_file->read({data, size});
}

usize FileHandle::write(void *data, usize size) const {
    return m_file->write({data, size});
}

bool FileHandle::valid() const {
    return m_file->valid();
}
