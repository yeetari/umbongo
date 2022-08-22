#pragma once

#include <kernel/SpinLock.hh>
#include <kernel/SysResult.hh>
#include <kernel/fs/File.hh> // IWYU pragma: keep
#include <kernel/fs/FileSystem.hh>
#include <kernel/fs/Inode.hh>
#include <kernel/fs/InodeType.hh>
#include <ustd/Optional.hh>
#include <ustd/SharedPtr.hh> // IWYU pragma: keep
#include <ustd/Span.hh>      // IWYU pragma: keep
#include <ustd/String.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>
#include <ustd/Vector.hh>

namespace kernel {

class DevFs;
class Device;

class DevFsInode final : public Inode {
    ustd::String m_name;

public:
    DevFsInode(const InodeId &id, const InodeId &parent, ustd::StringView name)
        : Inode(id, parent, InodeType::AnonymousFile), m_name(name) {}

    usize size() const override { return 0; }
    ustd::StringView name() const override { return m_name; }
};

class DevFsDirectoryInode final : public Inode {
    ustd::String m_name;
    ustd::Vector<InodeId> m_children;
    mutable SpinLock m_lock;

public:
    DevFsDirectoryInode(const InodeId &id, const InodeId &parent, ustd::StringView name)
        : Inode(id, parent, InodeType::Directory), m_name(name) {}

    SysResult<InodeId> child(usize index) const override;
    SysResult<InodeId> create(ustd::StringView name, InodeType type) override;
    InodeId lookup(ustd::StringView name) override;
    SysResult<> remove(ustd::StringView name) override;
    usize size() const override;
    ustd::StringView name() const override { return m_name; }
};

class DevFs final : public FileSystem {
    friend DevFsDirectoryInode;

private:
    ustd::LargeVector<ustd::UniquePtr<Inode>> m_inodes;

public:
    static void initialise();
    static void notify_attach(Device *device, ustd::StringView path);
    static void notify_detach(Device *device);

    void attach_device(Device *device, ustd::StringView path);
    void detach_device(Device *device);
    void mount(const InodeId &parent, const InodeId &host) override;
    Inode *inode(const InodeId &id) override;
};

} // namespace kernel
