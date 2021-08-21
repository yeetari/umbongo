#include <kernel/devices/DevFs.hh>

#include <kernel/devices/Device.hh>
#include <kernel/fs/File.hh>
#include <kernel/fs/Inode.hh>
#include <ustd/Assert.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Span.hh>
#include <ustd/String.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

// TODO: Needs proper locking.

namespace {

Vector<DevFs *> s_all;

} // namespace

void DevFs::notify_attach(Device *device) {
    for (auto *dev_fs : s_all) {
        dev_fs->attach_device(device);
    }
}

void DevFs::notify_detach(Device *device) {
    for (auto *dev_fs : s_all) {
        dev_fs->detach_device(device);
    }
}

DevFs::DevFs() {
    s_all.push(this);
    for (auto *device : Device::all_devices()) {
        attach_device(device);
    }
}

DevFs::~DevFs() {
    for (uint32 i = 0; i < s_all.size(); i++) {
        if (s_all[i] == this) {
            s_all.remove(i);
            return;
        }
    }
}

void DevFs::attach_device(Device *device) {
    m_root_inode.create(device->name(), device);
}

void DevFs::detach_device(Device *device) {
    m_root_inode.remove(device->name());
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

void DevFsRootInode::create(StringView name, Device *device) {
    m_children.emplace(name, device);
}

Inode *DevFsRootInode::create(StringView, InodeType) {
    ENSURE_NOT_REACHED();
}

Inode *DevFsRootInode::lookup(StringView name) {
    if (name == "." || name == "..") {
        return this;
    }
    for (auto &child : m_children) {
        if (name == child.name().view()) {
            return &child;
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
    for (uint32 i = 0; i < m_children.size(); i++) {
        if (name == m_children[i].name().view()) {
            m_children.remove(i);
            return;
        }
    }
}

usize DevFsRootInode::size() {
    return 0;
}

usize DevFsRootInode::write(Span<const void>, usize) {
    ENSURE_NOT_REACHED();
}
