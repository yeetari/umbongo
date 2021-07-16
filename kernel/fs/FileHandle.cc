#include <kernel/fs/FileHandle.hh>

uint64 FileHandle::read(void *data, usize size) const {
    return m_file->read({data, size});
}

uint64 FileHandle::write(void *data, usize size) const {
    return m_file->write({data, size});
}
