#pragma once

#include <kernel/devices/Device.hh>
#include <kernel/fs/File.hh> // IWYU pragma: keep
#include <kernel/fs/FileSystem.hh>
#include <kernel/fs/Inode.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Span.hh> // IWYU pragma: keep
#include <ustd/Span.hh>
#include <ustd/String.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

class DevFsInode final : public Inode {
    String m_name;
    SharedPtr<Device> m_device;

public:
    DevFsInode(StringView name, Device *device) : Inode(InodeType::Device), m_name(name), m_device(device) {}

    Inode *create(StringView name, InodeType type) override;
    Inode *lookup(StringView name) override;
    SharedPtr<File> open() override;
    usize read(Span<void> data, usize offset) override;
    void remove(StringView name) override;
    usize size() override;
    void truncate() override {}
    usize write(Span<const void> data, usize offset) override;

    const String &name() { return m_name; }
    const SharedPtr<Device> &device() { return m_device; }
};

class DevFsRootInode final : public Inode {
    Vector<DevFsInode> m_children;

public:
    DevFsRootInode() : Inode(InodeType::Directory) {}

    void create(StringView name, Device *device);
    Inode *create(StringView name, InodeType type) override;
    Inode *lookup(StringView name) override;
    SharedPtr<File> open() override;
    usize read(Span<void> data, usize offset) override;
    void remove(StringView name) override;
    usize size() override;
    void truncate() override {}
    usize write(Span<const void> data, usize offset) override;
};

class DevFs final : public FileSystem {
    DevFsRootInode m_root_inode;

public:
    static void notify_attach(Device *device);
    static void notify_detach(Device *device);

    DevFs();
    DevFs(const DevFs &) = delete;
    DevFs(DevFs &&) = delete;
    ~DevFs() override;

    DevFs &operator=(const DevFs &) = delete;
    DevFs &operator=(DevFs &&) = delete;

    void attach_device(Device *device);
    void detach_device(Device *device);

    Inode *root_inode() override { return &m_root_inode; }
};
