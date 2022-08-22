#pragma once

#include <kernel/SpinLock.hh>
#include <kernel/api/UserFs.hh>
#include <kernel/fs/File.hh>
#include <kernel/fs/FileSystem.hh>
#include <kernel/fs/Inode.hh>
#include <kernel/fs/InodeType.hh>
#include <ustd/Vector.hh>

namespace kernel {

class UserFs;

class UserFsInode final : public Inode {
    UserFs &fs() const;

public:
    UserFsInode(const InodeId &id, const InodeId &parent, InodeType type) : Inode(id, parent, type) {}

    SysResult<InodeId> child(usize index) const override;
    SysResult<InodeId> create(ustd::StringView name, InodeType type) override;
    InodeId lookup(ustd::StringView name) override;
    usize read(ustd::Span<void> data, usize offset) const override;
    SysResult<> remove(ustd::StringView name) override;
    usize size() const override;
    SysResult<> truncate() override;
    usize write(ustd::Span<const void> data, usize offset) override;
    ustd::StringView name() const override;
};

class UserFs final : public File, public FileSystem {
    friend UserFsInode;

private:
    ustd::Optional<UserFsInode> m_root_inode;
    ustd::LargeVector<char> m_pending_response;
    ustd::LargeVector<char> m_request_queue;
    mutable SpinLock m_lock;

    void send_request(const UserFsRequest &request, ustd::StringView view = {});
    UserFsResponse wait_response(ustd::LargeVector<char> &data);

public:
    bool read_would_block(usize offset) const override;
    bool write_would_block(usize offset) const override;
    SysResult<usize> read(ustd::Span<void> data, usize offset) override;
    SysResult<usize> write(ustd::Span<const void> data, usize offset) override;
    void mount(const InodeId &parent, const InodeId &host) override;
    Inode *inode(const InodeId &id) override;
};

} // namespace kernel
