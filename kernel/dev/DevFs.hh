#pragma once

#include <kernel/SpinLock.hh>
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

namespace kernel {

class Device;

class DevFsInode final : public Inode {
    ustd::String m_name;

public:
    DevFsInode(Inode *parent, ustd::StringView name) : Inode(InodeType::AnonymousFile, parent), m_name(name) {}

    Inode *child(usize index) override;
    Inode *create(ustd::StringView name, InodeType type) override;
    Inode *lookup(ustd::StringView name) override;
    ustd::SharedPtr<File> open_impl() override;
    usize read(ustd::Span<void> data, usize offset) override;
    void remove(ustd::StringView name) override;
    usize size() override;
    void truncate() override {}
    usize write(ustd::Span<const void> data, usize offset) override;
    ustd::StringView name() const override { return m_name; }
};

class DevFsDirectoryInode final : public Inode {
    ustd::String m_name;
    ustd::Vector<ustd::UniquePtr<Inode>> m_children;
    mutable SpinLock m_lock;

public:
    DevFsDirectoryInode(Inode *parent, ustd::StringView name) : Inode(InodeType::Directory, parent), m_name(name) {}

    Inode *child(usize index) override;
    Inode *create(ustd::StringView name, InodeType type) override;
    Inode *lookup(ustd::StringView name) override;
    ustd::SharedPtr<File> open_impl() override;
    usize read(ustd::Span<void> data, usize offset) override;
    void remove(ustd::StringView name) override;
    usize size() override;
    void truncate() override {}
    usize write(ustd::Span<const void> data, usize offset) override;
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
    Inode *root_inode() override { return m_root_inode.obj(); }
};

} // namespace kernel
