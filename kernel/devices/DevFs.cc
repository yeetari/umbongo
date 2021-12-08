#include <kernel/devices/DevFs.hh>

#include <kernel/ScopedLock.hh> // IWYU pragma: keep
#include <kernel/devices/Device.hh>
#include <kernel/fs/File.hh>
#include <kernel/fs/FileSystem.hh>
#include <kernel/fs/Inode.hh>
#include <kernel/fs/InodeType.hh>
#include <kernel/fs/Vfs.hh>
#include <ustd/Assert.hh>
#include <ustd/Numeric.hh> // IWYU pragma: keep
#include <ustd/Optional.hh>
#include <ustd/Result.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Span.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>
#include <ustd/Utility.hh>
#include <ustd/Vector.hh>

namespace {

DevFs *s_instance = nullptr;

} // namespace

void DevFs::initialise() {
    auto dev_fs = ustd::make_unique<DevFs>();
    s_instance = dev_fs.obj();
    Vfs::mkdir("/dev", nullptr);
    Vfs::mount("/dev", ustd::move(dev_fs));
    Vfs::mkdir("/dev/pci", nullptr);
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
    auto inode = Vfs::create(path, m_root_inode.obj(), InodeType::Device);
    ASSERT(!inode.is_error());
    static_cast<DevFsInode *>(*inode)->bind(device);
}

void DevFs::detach_device(Device *device) {
    m_root_inode->remove(device->path());
}

void DevFs::mount(Inode *parent, Inode *host) {
    ASSERT(!m_root_inode);
    m_root_inode.emplace(parent, host->name());
}

Inode *DevFsInode::child(usize) {
    ENSURE_NOT_REACHED();
}

Inode *DevFsInode::create(ustd::StringView, InodeType) {
    ENSURE_NOT_REACHED();
}

Inode *DevFsInode::lookup(ustd::StringView) {
    ENSURE_NOT_REACHED();
}

ustd::SharedPtr<File> DevFsInode::open_impl() {
    return m_device;
}

usize DevFsInode::read(ustd::Span<void>, usize) {
    ENSURE_NOT_REACHED();
}

void DevFsInode::remove(ustd::StringView) {
    ENSURE_NOT_REACHED();
}

usize DevFsInode::size() {
    return 0;
}

usize DevFsInode::write(ustd::Span<const void>, usize) {
    ENSURE_NOT_REACHED();
}

Inode *DevFsDirectoryInode::child(usize index) {
    ASSERT(index < ustd::Limits<uint32>::max());
    ScopedLock locker(m_lock);
    return m_children[static_cast<uint32>(index)].obj();
}

Inode *DevFsDirectoryInode::create(ustd::StringView name, InodeType type) {
    ScopedLock locker(m_lock);
    switch (type) {
    case InodeType::Device:
        return m_children.emplace(new DevFsInode(name, this)).obj();
    case InodeType::Directory:
        return m_children.emplace(new DevFsDirectoryInode(this, name)).obj();
    default:
        ENSURE_NOT_REACHED();
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
    for (auto &child : m_children) {
        if (name == child->name()) {
            return child.obj();
        }
    }
    return nullptr;
}

ustd::SharedPtr<File> DevFsDirectoryInode::open_impl() {
    ENSURE_NOT_REACHED();
}

usize DevFsDirectoryInode::read(ustd::Span<void>, usize) {
    ENSURE_NOT_REACHED();
}

void DevFsDirectoryInode::remove(ustd::StringView name) {
    ScopedLock locker(m_lock);
    for (uint32 i = 0; i < m_children.size(); i++) {
        if (name == m_children[i]->name()) {
            m_children.remove(i);
            return;
        }
    }
}

usize DevFsDirectoryInode::size() {
    ScopedLock locker(m_lock);
    return m_children.size();
}

usize DevFsDirectoryInode::write(ustd::Span<const void>, usize) {
    ENSURE_NOT_REACHED();
}
