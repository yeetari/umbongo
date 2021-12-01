#pragma once

#include <kernel/SpinLock.hh>
#include <kernel/devices/Device.hh>
#include <kernel/fs/File.hh> // IWYU pragma: keep
#include <kernel/fs/FileSystem.hh>
#include <kernel/fs/Inode.hh>
#include <kernel/fs/InodeType.hh>
#include <ustd/Optional.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Span.hh> // IWYU pragma: keep
#include <ustd/String.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>
#include <ustd/Vector.hh>

class DevFsInode final : public Inode {
    ustd::String m_name;
    ustd::SharedPtr<Device> m_device;

public:
    DevFsInode(ustd::StringView name, Inode *parent, Device *device)
        : Inode(InodeType::Device, parent), m_name(name), m_device(device) {}

    Inode *child(usize index) override;
    Inode *create(ustd::StringView name, InodeType type) override;
    Inode *lookup(ustd::StringView name) override;
    ustd::SharedPtr<File> open_impl() override;
    usize read(ustd::Span<void> data, usize offset) override;
    void remove(ustd::StringView name) override;
    usize size() override;
    void truncate() override {}
    usize write(ustd::Span<const void> data, usize offset) override;

    ustd::StringView name() const override { return m_name.view(); }
    const ustd::SharedPtr<Device> &device() { return m_device; }
};

class DevFsRootInode final : public Inode {
    ustd::String m_name;
    ustd::Vector<ustd::UniquePtr<DevFsInode>> m_children;
    mutable SpinLock m_lock;

public:
    DevFsRootInode(Inode *parent, ustd::StringView name) : Inode(InodeType::Directory, parent), m_name(name) {}

    Inode *child(usize index) override;
    void create(ustd::StringView name, Device *device);
    Inode *create(ustd::StringView name, InodeType type) override;
    Inode *lookup(ustd::StringView name) override;
    ustd::SharedPtr<File> open_impl() override;
    usize read(ustd::Span<void> data, usize offset) override;
    void remove(ustd::StringView name) override;
    usize size() override;
    void truncate() override {}
    usize write(ustd::Span<const void> data, usize offset) override;

    ustd::StringView name() const override { return m_name.view(); }
};

class DevFs final : public FileSystem {
    ustd::Optional<DevFsRootInode> m_root_inode;

public:
    static void notify_attach(Device *device);
    static void notify_detach(Device *device);

    DevFs() = default;
    DevFs(const DevFs &) = delete;
    DevFs(DevFs &&) = delete;
    ~DevFs() override;

    DevFs &operator=(const DevFs &) = delete;
    DevFs &operator=(DevFs &&) = delete;

    void attach_device(Device *device);
    void detach_device(Device *device);
    void mount(Inode *parent, Inode *host) override;
    Inode *root_inode() override { return m_root_inode.obj(); }
};
