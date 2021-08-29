#include <kernel/devices/DevFs.hh>

#include <kernel/ScopedLock.hh> // IWYU pragma: keep
#include <kernel/SpinLock.hh>
#include <kernel/devices/Device.hh>
#include <kernel/fs/File.hh>
#include <kernel/fs/Inode.hh>
#include <ustd/Assert.hh>
#include <ustd/Memory.hh>
#include <ustd/Numeric.hh> // IWYU pragma: keep
#include <ustd/Optional.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Span.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>
#include <ustd/Vector.hh>

namespace {

Vector<DevFs *> s_all;
SpinLock s_all_lock;

} // namespace

void DevFs::notify_attach(Device *device) {
    ScopedLock locker(s_all_lock);
    for (auto *dev_fs : s_all) {
        dev_fs->attach_device(device);
    }
}

void DevFs::notify_detach(Device *device) {
    ScopedLock locker(s_all_lock);
    for (auto *dev_fs : s_all) {
        dev_fs->detach_device(device);
    }
}

DevFs::~DevFs() {
    ScopedLock locker(s_all_lock);
    for (uint32 i = 0; i < s_all.size(); i++) {
        if (s_all[i] == this) {
            s_all.remove(i);
            return;
        }
    }
}

void DevFs::attach_device(Device *device) {
    m_root_inode->create(device->name(), device);
}

void DevFs::detach_device(Device *device) {
    m_root_inode->remove(device->name());
}

void DevFs::mount(Inode *parent, Inode *host) {
    ASSERT(!m_root_inode);
    m_root_inode.emplace(parent, host->name());
    {
        ScopedLock locker(s_all_lock);
        s_all.push(this);
    }
    for (auto *device : Device::all_devices()) {
        attach_device(device);
    }
}

Inode *DevFsInode::child(usize) {
    ENSURE_NOT_REACHED();
}

Inode *DevFsInode::create(StringView, InodeType) {
    ENSURE_NOT_REACHED();
}

Inode *DevFsInode::lookup(StringView) {
    ENSURE_NOT_REACHED();
}

SharedPtr<File> DevFsInode::open() {
    return m_device;
}

usize DevFsInode::read(Span<void> data, usize offset) {
    return m_device->read(data, offset);
}

void DevFsInode::remove(StringView) {
    ENSURE_NOT_REACHED();
}

usize DevFsInode::size() {
    return 0;
}

usize DevFsInode::write(Span<const void>, usize) {
    ENSURE_NOT_REACHED();
}

Inode *DevFsRootInode::child(usize index) {
    ASSERT(index < Limits<uint32>::max());
    ScopedLock locker(m_lock);
    return m_children[static_cast<uint32>(index)].obj();
}

void DevFsRootInode::create(StringView name, Device *device) {
    ScopedLock locker(m_lock);
    m_children.emplace(new DevFsInode(name, this, device));
}

Inode *DevFsRootInode::create(StringView, InodeType) {
    ENSURE_NOT_REACHED();
}

Inode *DevFsRootInode::lookup(StringView name) {
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

SharedPtr<File> DevFsRootInode::open() {
    ENSURE_NOT_REACHED();
}

usize DevFsRootInode::read(Span<void>, usize) {
    ENSURE_NOT_REACHED();
}

void DevFsRootInode::remove(StringView name) {
    ScopedLock locker(m_lock);
    for (uint32 i = 0; i < m_children.size(); i++) {
        if (name == m_children[i]->name()) {
            m_children.remove(i);
            return;
        }
    }
}

usize DevFsRootInode::size() {
    ScopedLock locker(m_lock);
    return m_children.size();
}

usize DevFsRootInode::write(Span<const void>, usize) {
    ENSURE_NOT_REACHED();
}
