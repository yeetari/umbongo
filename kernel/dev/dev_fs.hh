#pragma once

#include <kernel/fs/file_system.hh>
#include <kernel/fs/inode.hh>
#include <kernel/spin_lock.hh>
#include <kernel/sys_result.hh>
#include <ustd/optional.hh>
#include <ustd/string.hh>
#include <ustd/string_view.hh>
#include <ustd/types.hh>
#include <ustd/unique_ptr.hh>
#include <ustd/vector.hh>

namespace kernel {

class Device;

class DevFsInode final : public Inode {
    ustd::String m_name;

public:
    DevFsInode(Inode *parent, ustd::StringView name) : Inode(InodeType::AnonymousFile, parent), m_name(name) {}

    size_t size() const override { return 0; }
    ustd::StringView name() const override { return m_name; }
};

class DevFsDirectoryInode final : public Inode {
    ustd::String m_name;
    ustd::Vector<ustd::UniquePtr<Inode>> m_children;
    mutable SpinLock m_lock;

public:
    DevFsDirectoryInode(Inode *parent, ustd::StringView name) : Inode(InodeType::Directory, parent), m_name(name) {}

    SysResult<Inode *> child(size_t index) const override;
    SysResult<Inode *> create(ustd::StringView name, InodeType type) override;
    Inode *lookup(ustd::StringView name) override;
    SysResult<> remove(ustd::StringView name) override;
    size_t size() const override;
    ustd::StringView name() const override { return m_name; }
};

class DevFs final : public FileSystem {
    ustd::Optional<DevFsDirectoryInode> m_root_inode;

public:
    static void initialise();
    static void notify_attach(Device *device, ustd::StringView path);
    static void notify_detach(Device *device);

    void attach_device(Device *device, ustd::StringView path);
    void detach_device(Device *device);
    void mount(Inode *parent, Inode *host) override;
    Inode *root_inode() override { return m_root_inode.ptr(); }
};

} // namespace kernel
