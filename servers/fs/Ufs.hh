#pragma once

#include "Inode.hh"
#include "UfsTypes.hh"

#include <core/Error.hh>
#include <core/File.hh>
#include <core/UserFs.hh>
#include <ustd/Result.hh>
#include <ustd/Span.hh>
#include <ustd/Vector.hh>

namespace core {

class EventLoop;

} // namespace core

class Drive;
class Ufs;

class UfsInode {
    Ufs &m_fs;
    const ufs::BlockIndex m_block_index;
    const ufs::InodeHeader m_header;

public:
    UfsInode(Ufs &fs, ufs::BlockIndex block_index, ufs::InodeHeader header)
        : m_fs(fs), m_block_index(block_index), m_header(header) {}

    usize read_bytes(ustd::Span<void> data, usize offset);
    void traverse_directory();

    ufs::BlockIndex block_index() const { return m_block_index; }
    bool is_directory() const { return m_header.type == ufs::InodeType::Directory; }
};

class Ufs {
    friend UfsInode; // TODO: Maybe remove.

    Drive &m_drive;
    core::File m_user_fs;
    ustd::Vector<ufs::GroupHeader, ufs::GroupIndex> m_group_headers;
    InodePtr<UfsInode> m_root_inode;
    uint32 m_inodes_per_group{0};
    uint32 m_block_indices_per_inode{0};
    uint16 m_block_size{0};

    InodePtr<UfsInode> read_inode(ufs::BlockIndex index);

public:
    explicit Ufs(Drive &drive) : m_drive(drive) {}

    ustd::Result<void, core::SysError> initialise(core::EventLoop &event_loop);
    uint16 block_size() const { return m_block_size; }
    Drive &drive() { return m_drive; } // TODO: Remove.
};
