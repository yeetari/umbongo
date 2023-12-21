#include <kernel/dev/dev_fs.hh>

#include <kernel/dev/device.hh>
#include <kernel/error.hh>
#include <kernel/fs/file.hh>
#include <kernel/fs/file_system.hh>
#include <kernel/fs/inode.hh>
#include <kernel/fs/vfs.hh>
#include <kernel/scoped_lock.hh>
#include <kernel/spin_lock.hh>
#include <kernel/sys_result.hh>
#include <ustd/assert.hh>
#include <ustd/numeric.hh>
#include <ustd/optional.hh>
#include <ustd/shared_ptr.hh>
#include <ustd/string_view.hh>
#include <ustd/try.hh>
#include <ustd/types.hh>
#include <ustd/unique_ptr.hh>
#include <ustd/utility.hh>
#include <ustd/vector.hh>

namespace kernel {
namespace {

DevFs *s_instance = nullptr;

} // namespace

void DevFs::initialise() {
    auto dev_fs = ustd::make_unique<DevFs>();
    s_instance = dev_fs.ptr();
    EXPECT(Vfs::mkdir("/dev", nullptr));
    EXPECT(Vfs::mount("/dev", ustd::move(dev_fs)));
    EXPECT(Vfs::mkdir("/dev/pci", nullptr));
}

void DevFs::notify_attach(Device *device, ustd::StringView path) {
    ASSERT_PEDANTIC(s_instance != nullptr);
    s_instance->attach_device(device, path);
}

void DevFs::notify_detach(Device *device) {
    ASSERT_PEDANTIC(s_instance != nullptr);
    s_instance->detach_device(device);
}

void DevFs::attach_device(Device *device, ustd::StringView path) {
    auto *inode = EXPECT(Vfs::create(path, m_root_inode.ptr(), InodeType::AnonymousFile));
    inode->bind_anonymous_file(ustd::SharedPtr<Device>(device));
}

void DevFs::detach_device(Device *device) {
    // TODO: Should be Vfs::delete for full path resolution.
    EXPECT(m_root_inode->remove(device->path()));
}

void DevFs::mount(Inode *parent, Inode *host) {
    ASSERT(!m_root_inode);
    m_root_inode.emplace(parent, host->name());
}

SysResult<Inode *> DevFsDirectoryInode::child(size_t index) const {
    if (index >= ustd::Limits<uint32_t>::max()) {
        return Error::Invalid;
    }
    ScopedLock locker(m_lock);
    return m_children[static_cast<uint32_t>(index)].ptr();
}

SysResult<Inode *> DevFsDirectoryInode::create(ustd::StringView name, InodeType type) {
    ScopedLock locker(m_lock);
    switch (type) {
    case InodeType::AnonymousFile:
        return m_children.emplace(new DevFsInode(this, name)).ptr();
    case InodeType::Directory:
        return m_children.emplace(new DevFsDirectoryInode(this, name)).ptr();
    default:
        return Error::Invalid;
    }
}

Inode *DevFsDirectoryInode::lookup(ustd::StringView name) {
    if (name == ".") {
        return this;
    }
    if (name == "..") {
        return parent();
    }
    ScopedLock locker(m_lock);
    for (const auto &child : m_children) {
        if (child->name() == name) {
            return child.ptr();
        }
    }
    return nullptr;
}

SysResult<> DevFsDirectoryInode::remove(ustd::StringView name) {
    ScopedLock locker(m_lock);
    for (uint32_t i = 0; i < m_children.size(); i++) {
        if (m_children[i]->name() == name) {
            m_children.remove(i);
            return {};
        }
    }
    return Error::NonExistent;
}

size_t DevFsDirectoryInode::size() const {
    ScopedLock locker(m_lock);
    return m_children.size();
}

} // namespace kernel
