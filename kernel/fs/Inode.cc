#include <kernel/fs/Inode.hh>

#include <kernel/fs/File.hh>
#include <kernel/fs/InodeType.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Utility.hh>

void Inode::bind_ipc_file(ustd::SharedPtr<File> ipc_file) {
    m_ipc_file = ustd::move(ipc_file);
}

ustd::SharedPtr<File> Inode::open() {
    if (m_type == InodeType::IpcFile) {
        return m_ipc_file;
    }
    return open_impl();
}
