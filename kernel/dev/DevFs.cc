#include <kernel/dev/DevFs.hh>

#include <kernel/ScopedLock.hh> // IWYU pragma: keep
#include <kernel/SysError.hh>
#include <kernel/SysResult.hh>
#include <kernel/dev/Device.hh>
#include <kernel/fs/File.hh>
#include <kernel/fs/FileSystem.hh>
#include <kernel/fs/Inode.hh>
#include <kernel/fs/InodeType.hh>
#include <kernel/fs/Vfs.hh>
#include <ustd/Assert.hh>
#include <ustd/Numeric.hh> // IWYU pragma: keep
#include <ustd/Optional.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/StringView.hh>
#include <ustd/Try.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>
#include <ustd/Utility.hh>
#include <ustd/Vector.hh>

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

SysResult<Inode *> DevFsDirectoryInode::child(usize index) const {
    if (index >= ustd::Limits<uint32>::max()) {
        return SysError::Invalid;
    }
    ScopedLock locker(m_lock);
    return m_children[static_cast<uint32>(index)].ptr();
}

SysResult<Inode *> DevFsDirectoryInode::create(ustd::StringView name, InodeType type) {
    ScopedLock locker(m_lock);
    switch (type) {
    case InodeType::AnonymousFile:
        return m_children.emplace(new DevFsInode(this, name)).ptr();
    case InodeType::Directory:
        return m_children.emplace(new DevFsDirectoryInode(this, name)).ptr();
    default:
        return SysError::Invalid;
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
    for (uint32 i = 0; i < m_children.size(); i++) {
        if (m_children[i]->name() == name) {
            m_children.remove(i);
            return {};
        }
    }
    return SysError::NonExistent;
}

usize DevFsDirectoryInode::size() const {
    ScopedLock locker(m_lock);
    return m_children.size();
}

} // namespace kernel